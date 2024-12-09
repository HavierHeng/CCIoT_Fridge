import asyncio
import aiohttp
import aiofiles
import xml.etree.ElementTree as ET
import os
import argparse

async def download_file(protocol, hostname, path, file_path, chunk_size=64*1024):
    """
    Asynchronously downloads files from S3 bucket. Chunks data to be downloaded.
    The parameters are split from the signed URL.

    Protocol - http/https
    Hostname - The domain of the signed url (e.g fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com)
    Path - The path of the signed URL to download from (e.g /audio/test.wav)
    File Path - The path to download file to

    Chunk size for S3 is minimum 8kB, recommended 64kB.
    """
    async with aiohttp.ClientSession(protocol + "://" + hostname) as session:
        async with session.get(path) as response: 
            if response.status == 200:
                print("Downloading file...")
            async with aiofiles.open(file_path, 'wb') as fd:
                async for chunk in response.content.iter_chunked(chunk_size):
                    await fd.write(chunk)
    return

async def upload_file(protocol, hostname, path, file_path, chunk_size=64*1024, file_type="audio/wav"):
    """
    Asynchronously upload files to s3 bucket. chunks data to be uploaded.
    The parameters are split from the signed url.

    Protocol - http/https
    Hostname - the domain of the signed url (e.g fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com)
    Path - the path of the signed url to upload to (e.g /audio/test.wav)
    File path - the path to the file to be uploaded 

    Chunk size for S3 is minimum 8kB, recommended 64kB.
    """
    async def file_sender():
        async with aiofiles.open(file_path, 'rb') as f:
            chunk = await f.read(chunk_size)
            while chunk:
                yield chunk
                chunk = await f.read(chunk_size) 

    # Host is auto added by aiohttp
    # Content-Length is needed as AWS does not support Transfer-Encoding: chunked
    headers = {"Content-Type": file_type,
               "Content-Length": str(os.path.getsize(file_path))} 

    async with aiohttp.ClientSession(protocol + "://" + hostname) as session:
        async with session.put(path, data=file_sender(), headers=headers) as response:
            if response.status == 200:
                print("Upload completed successfully")


def parse_upload_id(response_text):
    """
    AWS CreateMultiPartUpload goes by this format:

    <?xml version="1.0" encoding="UTF-8"?>
    <InitiateMultipartUploadResult>
       <Bucket>string</Bucket>
       <Key>string</Key>
       <UploadId>string</UploadId>
    </InitiateMultipartUploadResult>

    """
    # Parse the XML response text to extract the UploadId
    root = ET.fromstring(response_text)  
    upload_id = root.find('UploadId')
    if upload_id:
        return upload_id.text

def generate_complete_body(parts):
    """
    Generate the XML body to complete the upload, which includes all part numbers and ETags
    """
    body = "<CompleteMultipartUpload>"
    for part in parts:
        body += f"<Part><PartNumber>{part['PartNumber']}</PartNumber><ETag>{part['ETag']}</ETag></Part>"
    body += "</CompleteMultipartUpload>"
    return body

async def multipart_upload_file(protocol, hostname, path, file_path, chunk_size=64*1024, file_type="audio/wav"):
    """
    Asynchronously uploads files from S3 bucket. 
    Uses AWS Multipart Upload: https://docs.aws.amazon.com/AmazonS3/latest/userguide/mpuoverview.html
    
    The order of operations are as follows:
    1) CreateMultiPartUpload - POST to get the Multipart 
    2) UploadPart - PUT as many times as needed to upload the file
    3) CompleteMultiPartUpload - Indicate end of multipart upload

    The parameters are split from the signed URL.

    Protocol - http/https
    Hostname - The domain of the signed url (e.g fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com)
    Path - The path of the signed URL to upload to (e.g /audio/test.wav)
    File Path - The path to file to be uploaded

    To Note: Anonymous or Free Tier users cannot use this.
    """
    async def file_sender():
        async with aiofiles.open(file_path, 'rb') as f:
            chunk = await f.read(chunk_size)
            while chunk:
                yield chunk
                chunk = await f.read(chunk_size)

    # CreateMultipartUpload: Initiate the Multipart Upload on S3
    async with aiohttp.ClientSession() as session:
        upload_url = f"{protocol}://{hostname}{path}?uploads"
        headers = {
            "Content-Type": "application/xml",
        }
        async with session.post(upload_url, headers=headers) as response:
            if response.status != 200:
                print(f"Failed to initiate upload: {response.status}")
                return
            
            # Parse the response to get the UploadId
            response_text = await response.text()
            upload_id = parse_upload_id(response_text)
            print(f"Upload initiated successfully. Upload ID: {upload_id}")

        # UploadPart: Upload file parts, AWS accepts up to 1000 UploadPart Calls
        part_number = 1
        parts = []
        async with aiohttp.ClientSession() as session:
            async for chunk in file_sender():
                part_url = f"{protocol}://{hostname}{path}?partNumber={part_number}&uploadId={upload_id}"
                headers = {"Content-Type": file_type}
                async with session.put(part_url, data=chunk, headers=headers) as part_response:
                    if part_response.status != 200:
                        print(f"Failed to upload part {part_number}: {part_response.status}")
                        return
                    
                    # Get ETag for part (useful for completing the upload later)
                    part_etag = await part_response.text()
                    parts.append({"PartNumber": part_number, "ETag": part_etag})
                    print(f"Uploaded part {part_number}")
                    part_number += 1

        # CompleteMultipartUpload: Complete the upload
        complete_url = f"{protocol}://{hostname}{path}?uploadId={upload_id}"
        body = generate_complete_body(parts)
        headers = {"Content-Type": "application/xml"}
        async with session.post(complete_url, data=body, headers=headers) as complete_response:
            if complete_response.status == 200:
                print("Upload completed successfully")
            else:
                print(f"Failed to complete upload: {complete_response.status}")

async def handle_operation(operation, protocol, domain, path, file_name):
    if operation == "upload":
        await upload_file(protocol, domain, path, file_name)
    elif operation == "download":
        await download_file(protocol, domain, path, file_name)
    elif operation == "multipart_upload":
        await multipart_upload_file(protocol, domain, path, file_name)
    else:
        print(f"Unknown operation: {operation}")

def parse_args():
    parser = argparse.ArgumentParser(description="File transfer operations")
    # Define arguments
    parser.add_argument("operation", choices=["upload", "download", "multipart_upload"],
                        help="Operation to perform: upload, download, or multipart_upload")
    parser.add_argument("protocol", choices=["http", "https"],
                        help="Protocol to use for the transfer")
    parser.add_argument("hostname", help="Hostname or IP address")
    parser.add_argument("path", help="Path on the remote server")
    parser.add_argument("file_name", help="File name to upload/download")
    return parser.parse_args()

def main():
    args = parse_args()
    asyncio.run(handle_operation(args.operation, args.protocol, args.hostname, args.path, args.file_name))

if __name__ == "__main__":
    main()

