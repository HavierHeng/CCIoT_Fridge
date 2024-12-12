import boto3
from botocore.client import Config
import json
import uuid
from urllib.parse import urlparse

def lambda_handler(event, context):
    bucket_name = 'fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e'
    file_name = str(uuid.uuid4()) + '.wav'

    mqtt = boto3.client('iot-data', region_name='ap-southeast-1')
    s3 = boto3.client('s3',config=Config(signature_version='s3v4'))

    url = s3.generate_presigned_url('put_object', Params={'Bucket':bucket_name, 'Key':"audio/item"}, ExpiresIn=60000, HttpMethod='PUT')
    print(url)
    parsed = urlparse(url)
    data = {
        "protocol": parsed.scheme,
        "hostname": parsed.hostname,
        "path": parsed.path+"/"+file_name,
        "query": parsed.query
    }

    # command = "curl --request PUT --upload-file {} '{}'".format(file_name, url)
    # print(command) # for local testing purpose
    # print(file_name + '/' + url[8:]) # for local testing purposes

    response = mqtt.publish(
            topic='espfridge/signedurl',
            qos=1,
            payload=json.dumps(data)
        )