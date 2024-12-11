from datetime import datetime
VALID_ITEMS = [
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

def verify_item(s: str):
    normalized = s.lower()
    for i in VALID_ITEMS:
        if i in normalized:
            return True 
    return False

def verify_date(s: str):
    try:
        # Try parsing the date string to a datetime object
        datetime.strptime(s, '%d/%m/%y')
        return True  # Date is valid
    except ValueError:
        # If a ValueError is raised, it's an invalid date (e.g., 31/02/24)
        return False
