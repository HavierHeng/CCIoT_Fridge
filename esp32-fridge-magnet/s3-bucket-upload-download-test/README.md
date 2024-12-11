# Download/Uploading to S3 Bucket

Why does AWS not document how to upload shit to S3? I don't want to use their client library, I just want to know to REST API to their presigned URL.

Rant over. This side project is just to test PUT and GET because its deceivingly simple. In reality, dealing with memory limitations will make a grown man cry. 

It also tests my patience with AWS and ESP-IDF. Their example codes suck is too convoluted to even be useful. I instead opt to use a more proper method whereby I have no dependencies on any one cloud provider embedded SDK - so esp-http-client.

There is some basic cJSON stuff used here just to see if I can format and parse JSON.


# Menuconfig changes
i2c screen
partition size
sd card pins
fatfs long names

## POST
This side side project tests how to upload stuff to an AWS S3 Bucket via a chunked PUT request. No MQTT, no mess from anything else. All I want is a fair upload to PORT 80 with the relevant PUT path, Host fields.

The big challenge in embedded is that there is literally no memory. I am using an SD card which I upload in chunks from. Else it would be like the lab where uploading a picture to the S3 bucket causes it to explode due to too much heap usage.

Also instead of being like the lab, and manually resolving DNS, opening a raw TCP socket, then writing the file altogether, I'm going to use the esp-http-client library to properly help me make the packets.

This has a few benefits:
- No need to manually resolve DNS via lwip - `getaddrinfo()`
- No need to manually craft the HTTP PUT Header
- Imagine opening socket

What I know about the format so far:
- Connect port 80
- Send header - AWS needs these - new lines are split by CLRF
    - PUT /%s HTTP/1.1\r\n
        - Put as Path to resource
    - Host: %s\r\n 
        - Put as AWS bucket domain name
    - Content-Type: application/x-www-form-urlencoded\r\n
    - Content-Length: %d\r\n
    - \r\n
- Send body - you can keep sending up till Content-Length is reached
    - even as partial uploads
- Send footer - `\r\n` to indicate data end

In short its this format using a presigned URL from S3:
```
PUT /<path/key> HTTP/1.1\r\n
Host: Bucket.s3.amazonaws.com\r\n
Content-Type: audio/wav\r\n
Content-Length: %d\r\n

<Data section>

\r\n
```

In esp-http-client - I can "open" the connection and then keep spamming write to send data in chunks.

In the lab example, `application/x-www-form-urlencoded` is not actually optimal as the encoding type does actually slows uploading since it percent encodes binary data, effectively making 1 byte of binary data to 3 bytes. But in reality, AWS being AWS secretly doesn't use this field at all, it only uses it for metadata.

AWS Docs on S3 uploading objects: https://docs.aws.amazon.com/AmazonS3/latest/userguide/upload-objects.html
> Upload an object in a single operation by using the AWS SDKs, REST API, or AWS CLI – With a single PUT operation, you can upload a single object up to 5 GB in size.

AWS PutObject Docs: https://docs.aws.amazon.com/AmazonS3/latest/API/API_PutObject.html

Their example PUT request
```
PUT /my-image.jpg HTTP/1.1
Host: myBucket.s3.<Region>.amazonaws.com
Date: Wed, 12 Oct 2009 17:50:00 GMT
Authorization: authorization string
Content-Type: text/plain
Content-Length: 11434
x-amz-meta-author: Janet
Expect: 100-continue
```
- In my case, I don't have Expect since I don't care nor want to deal with handling 100-continue responses before sending
- I don't have an authorization string since its presigned
- AWS apparently doesn't care about Content-Type is what I've learnt from the labs and here. It also does not care if I percent encode my data if it `application/x-www-form-urlencoded`
    - This is true from this stack overflow thread: https://stackoverflow.com/questions/36301483/what-does-amazon-s3-use-the-content-type-header-for
    - The `Content-Type` here just affects how AWS S3 Tags metadata - in my case is meant to `audio/wav`


## PUT via CURL test
If my IAM permissions are set to all can upload then all i need is the path to file without any of the AWS headers
- `curl --request PUT --upload-file metal_pipe.wav https://fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com/audio/expiry/itembaa97d7f-5fda-488f-811b-3dcb3bd38304.wav`
	- This works fine
	- Yeah and if i upload i overwrite the old file, it obeys idempotency


`Content-Type` only affects the return object when I GET via the presigned URL later on.
- Its pretty non-standard HTTP PUT behavior
- So like when I do `curl 'https://fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e.s3.amazonaws.com/audio/expiry/itembaa97d7f-5fda-488f-811b-3dcb3bd38304.wav' --output test.wav -v`
    - It states `Content-Type` is `application/json` if it was uploaded as `application/json`
    - The reason why is as it is stored in Metadata in S3 bucket

## GET Request
It tests downloading a wav file via GET request.

In this case, it is to use `esp-http-client` callbacks to slowly upload the file. I also make use of userdata to hold onto the file pointer on http connect.




# Some gimmicks

Stack size needs to be large enough. This involves making another task altogether.


Conclusion. I give up. 

Errors I faced where
1) For PUT, since its so slow, the server usually errors or gives up before it actually finishes
2) GET has some funny issues with synchronization due to callbacks. For some reason it doesn't want to fully download either.
