import boto3
from botocore.client import Config
import json
from datetime import datetime
import requests
import io

tts_api = "http://10.0.155.166/tts"
recipe_api = "http://10.0.155.166/recipe"
# tts_api = "http://10.0.152.102/tts"
# recipe_api = "http://10.0.152.102/recipe"

bucket_name = "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e"


def parse_date(date_string):
    """
    Parse date string in dd/mm/yyyy format
    """
    return datetime.strptime(date_string, '%d/%m/%Y')


def lambda_handler(event, context):
    print('STARTED')
    # Initialize S3 client
    """This function will create the url to access the wav file from the S3 and publish it via MQTT """
    # define the name of the endpoints and bucket
    # default_endpoint = "a1il69uezpqouq-ats.iot.ap-southeast-1.amazonaws.com"
    # endpoint = "d06862398xmnxt64uosw-ats.iot.ap-southeast-1.amazonaws.com"
    # Initialize DynamoDB client
    dynamodb = boto3.resource('dynamodb', region_name='ap-southeast-1')

    s3_client = boto3.client('s3', region_name='ap-southeast-1')
    print("Got s3 client")
    try:
        table = dynamodb.Table('iot-items')
        print('REQUESTING DYANAMO table')
        response = table.scan()
        items = response.get('Items', [])
        print(items)
        sorted_items = sorted(
            items,
            key=lambda x: parse_date(x.get('expiry', '01/01/1970'))
        )
        first10 = sorted_items[:10]

        items = [item.get('name', '') for item in first10]

        print('FILTERED', items)

        # Make request to recipe API
        recipe_response = requests.get(
            f"{recipe_api}?ingredients={','.join(items)}"
        )
        print("Finishn rq")

        recipe_response.raise_for_status()
        recipe_data = recipe_response.json()
        print(recipe_data)
        suggestions = recipe_data.get('suggestions', [])

        audio = get_tts(
            f"Here are some suggested recipes! {suggestions[0]}, {suggestions[1]}, and {suggestions[2]}. Enjoy!",
            'tts/suggested-recipes.wav',
            s3_client,
        )
        text = 'Top recipe:     ' + (suggestions[0] if len(suggestions[0]) <= 16 else (suggestions[0][:13] + '...'))
        print(text, audio)
        mqtt_topic = "espfridge/tts"
        payload = json.dumps({"audio": audio, "text": text})
        iot_client = boto3.client('iot-data', region_name='ap-southeast-1')

        print(payload)
        # Debugging print
        print("Publishing to MQTT...\n")
        # Publish the URL to the MQTT topic
        response = iot_client.publish(
            topic=mqtt_topic,
            qos=1,
            payload=payload
        )
        return {
            'statusCode': 200,
            'body': f"File URL {audio} successfully published to {mqtt_topic}"
        }
    except Exception as e:
        print(f"Error: {e}")
        return {
            'statusCode': 500,
            'body': str(e)
        }


def get_tts(text, file, s3_client):
    print(text, file)
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

