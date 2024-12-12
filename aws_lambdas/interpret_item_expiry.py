import boto3
from botocore.client import Config
import json
import datefinder
import requests

# s2t_api = "http://10.0.20.114/s2t"
s2t_api = "http://10.0.155.166/s2t"


def lambda_handler(event, context):
    # Initialize S3 client
    s3_client = boto3.client('s3', region_name='ap-southeast-1')
    """This function will create the url to access the wav file from the S3 and publish it via MQTT """
    # define the name of the endpoints and bucket
    bucket_name = "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e"
    # default_endpoint = "a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com"
    # endpoint = "d06862398xmnxt64uosw-ats.iot.ap-southeast-1.amazonaws.com"
    print(event)
    try:
        bucket_name = event['Records'][0]['s3']['bucket']['name']
        object_key = event['Records'][0]['s3']['object']['key']
        print(bucket_name, object_key)
        # Check file extension to ensure it's an audio file
        if not object_key.endswith(('.mp3', '.wav')):
            print(f"Skipping non-audio file: {object_key}")
            return
        response = s3_client.get_object(Bucket=bucket_name, Key=object_key)
        file_bytes = response['Body'].read()
        files = {
            'file': ('file', file_bytes)  # key is the original filename
        }

        s2t_resp = requests.post(
            s2t_api,
            files=files,
        )

        print(s2t_resp.text)

        if s2t_resp.status_code != 200:
            print(f"S2T upload failed: {s2t_resp.text}")
            return {
                'statusCode': s2t_resp.status_code,
                'body': json.dumps('API upload failed')
            }

        expiry = verify(s2t_resp.text)
        print(expiry)
        if expiry is None:
            return {
                'statusCode': 200,
                'body': json.dumps('No text found')
            }

        # Define MQTT topic and payload
        mqtt_topic = "espfridge/tts"
        payload = json.dumps({"audio": '', "text": expiry})
        iot_client = boto3.client('iot-data', region_name='ap-southeast-1')
        print(payload)

        # Debugging print
        # print("Publishing to MQTT...\n")
        # Publish the URL to the MQTT topic
        response = iot_client.publish(
            topic=mqtt_topic,
            qos=1,
            payload=payload
        )
        return {
            'statusCode': 200,
            'body': f"Expiry {expiry} successfully published to {mqtt_topic}"
        }
    except Exception as e:
        print(f"Error: {e}")
        return {
            'statusCode': 500,
            'body': str(e)
        }


def verify(s: str):
    matches = datefinder.find_dates(s)
    for m in matches:
        return m.strftime('%d/%m/%y')

