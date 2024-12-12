import json
import boto3


def lambda_handler(event, context):
    print('RECV EVENT', event)
    dynamodb = boto3.resource('dynamodb', region_name='ap-southeast-1')
    try:
        # Validate required fields
        required_fields = ['item', 'expiry', 'weight']
        for field in required_fields:
            if field not in event:
                raise ValueError(f"Missing required field: {field}")

        # Print out the item details
        print(f"New Item Added:")
        print(f"Item: {event['item']}")
        print(f"Expiry Date: {event['expiry']}")
        print(f"Weight: {event['weight']}")

        table = dynamodb.Table('iot-items')
        db_item = {
            'name': event['item'],
            'expiry': event['expiry'],
            'weight': event['weight']
        }

        response = table.put_item(
            Item=db_item,
        )
        print(response)
        return {
            'statusCode': 200,
            'body': json.dumps({
                'message': 'Item processed successfully',
                'item': event['item']
            })
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