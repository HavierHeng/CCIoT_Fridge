import boto3
from botocore.client import Config
import json, io
import requests
import uuid

tts_api = "http://10.0.155.166/tts"
s2t_api = "http://10.0.155.166/s2t"
# tts_api = "http://10.0.152.102/tts"
# s2t_api = "http://10.0.152.102/s2t"
bucket_name = "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e"


def lambda_handler(event, context):
    # Initialize S3 client
    s3_client = boto3.client('s3', region_name='ap-southeast-1')
    """This function will create the url to access the wav file from the S3 and publish it via MQTT """
    # define the name of the endpoints and bucket
    # default_endpoint = "a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com"
    # endpoint = "d06862398xmnxt64uosw-ats.iot.ap-southeast-1.amazonaws.com"
    print(event)
    try:
        # TODO 8-12
        bucket_name = event['Records'][0]['s3']['bucket']['name']
        object_key = event['Records'][0]['s3']['object']['key']
        print(bucket_name, object_key)

        # Check file extension to ensure it's an audio file
        if not object_key.endswith(('.mp3', '.wav')):
            print(f"Skipping non-audio file: {object_key}")
            return

        print('HEREA')
        response = s3_client.get_object(Bucket=bucket_name, Key=object_key)
        file_bytes = response['Body'].read()
        files = {
            'file': ('file', file_bytes)  # key is the original filename
        }

        print('GOT OBJECT')
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

        item = verify(s2t_resp.text)
        if item is None:
            return {
                'statusCode': 200,
                'body': json.dumps('No text found')
            }

        tts_resp = get_tts(
            f'When does {item} expire?',
            f'tts/{uuid.uuid4()}.wav',
            s3_client,
        )

        # Print the body as text
        print(tts_resp)

        # # Construct the S3 object URL
        # print(f"Generated S3 URL: {s3_url}")

        # Define MQTT topic and payload
        mqtt_topic = "espfridge/tts"
        payload = json.dumps({"audio": tts_resp, "text": item})
        print(payload)
        iot_client = boto3.client('iot-data', region_name='ap-southeast-1')

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
            'body': f"File URL {tts_resp} successfully published to {mqtt_topic}"
        }
    except Exception as e:
        print(f"Error: {e}")
        return {
            'statusCode': 500,
            'body': str(e)
        }


def get_tts(text, file, s3_client):
    tts_resp = requests.post(
        tts_api,
        data=json.dumps({'text': text}),
        headers={'Content-type': 'application/json'}
    )
    tts_resp.raise_for_status()

    # Upload the bytes directly to S3
    s3_client.upload_fileobj(
        io.BytesIO(tts_resp.content),  # Convert bytes to file-like object
        bucket_name,
        file
    )
    return f"https://{bucket_name}.s3.amazonaws.com/{file}"


# RANDOM WORD LIST
WORD_LIST = [
    'beans',
    'cake',
    'candy',
    'cereal',
    'chips',
    'chocolate',
    'coffee',
    'corn',
    'fish',
    'flour',
    'honey',
    'jam',
    'juice',
    'milk',
    'nuts',
    'oil',
    'pasta',
    'rice',
    'soda',
    'spices',
    'sugar',
    'tea',
    'tomato',
    'vinegar',
    'water',
    'coke'
]


# by right this should use an AI model to entity recognition
def verify(s: str):
    normalized = s.lower()
    for i in WORD_LIST:
        if i in normalized:
            return i
