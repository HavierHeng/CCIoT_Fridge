import json, io
import boto3
import requests
from boto3.dynamodb.conditions import Key
from decimal import Decimal
from datetime import datetime

bucket_name = "fridge-bucket-a8e757a7-74fc-48fd-9391-ee3bd3ebf30e"


def lambda_handler(event, context):
    print(event)
    dynamodb = boto3.resource('dynamodb', region_name='ap-southeast-1')
    try:
        print('GEt dynamo table')
        item_weight = Decimal(float(event))

        table = dynamodb.Table('iot-items')

        response = table.query(
            KeyConditionExpression=Key('weight').eq(item_weight)
        )

        items = response.get('Items', [])
        print('Found items', items)
        audio = ''
        text = ''
        if len(items) != 1:
            print('HERERER')
            s3_client = boto3.client('s3', region_name='ap-southeast-1')
            audio = f"https://{bucket_name}.s3.amazonaws.com/tts/error-consume.wav"
            text = "Error! Failed to remove item."
        else:
            response = table.delete_item(
                Key={
                    'weight': item_weight
                }
            )
            expiry_date = datetime.strptime(items[0]['expiry'], '%d/%m/%Y').date()
            today = datetime.now().date()
            text_base = 'Removing item:  '
            if expiry_date < today:
                text_base = 'EXPIRED ITEM!!  '
            text = text_base + items[0]['name']

        mqtt_topic = "espfridge/tts"
        payload = json.dumps({"audio": audio, "text": text})
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
            'body': json.dumps('Item processed successfully'),
        }

    except json.JSONDecodeError:
        print("Error: Invalid JSON format")
        return {
            'statusCode': 400,
            'body': json.dumps({'error': 'Invalid JSON format'})
        }
    except ValueError as ve:
        print(f"Validation Error: {str(ve)}")
        return {
            'statusCode': 400,
            'body': json.dumps({'error': str(ve)})
        }
    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        return {
            'statusCode': 500,
            'body': json.dumps({'error': 'Internal server error'})
        }
