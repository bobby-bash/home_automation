import cv2
import mediapipe as mp

import requests
import json

url = 'https://apple.sinric.pro/v1/shortcuts/actions'
headers = {
    'Content-Type': 'application/json'
}
data1 = {
    'api_key': '3db612e7-ccc3-4bbb-bb00-d456830e49c7',
    'device_id': '6611818d7c9e6c6fe865bec7',
    'action': 'setPowerState',
    'value': {
        'state': 'Off'
    }
}

data2 = {
    'api_key': '3db612e7-ccc3-4bbb-bb00-d456830e49c7',
    'device_id': '6611818d7c9e6c6fe865bec7',
    'action': 'setPowerState',
    'value': {
        'state': 'On'
    }
}


mp_drawing = mp.solutions.drawing_utils
mp_hands = mp.solutions.hands

# Function to count fingers
def count_fingers(hand_landmarks):
    # Thumb
    if hand_landmarks.landmark[4].y < hand_landmarks.landmark[3].y:
        thumb_open = True
    else:
        thumb_open = False

    # Other fingers
    fingers = 0
    for finger in range(1, 5):
        if hand_landmarks.landmark[finger * 4].y < hand_landmarks.landmark[finger * 4 - 2].y:
            fingers += 1

    if thumb_open:
        fingers += 1

    return fingers

# Main function
def main():
    cap = cv2.VideoCapture(0)

    with mp_hands.Hands(
        min_detection_confidence=0.5,
        min_tracking_confidence=0.5) as hands:
        while cap.isOpened():
            ret, frame = cap.read()
            if not ret:
                continue

            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = hands.process(frame_rgb)

            if results.multi_hand_landmarks:
                for hand_landmarks in results.multi_hand_landmarks:
                    mp_drawing.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

                    fingers = count_fingers(hand_landmarks)
                    temp = 0
                    cv2.putText(frame, f"Fingers: {fingers}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                    if(temp != fingers):
                        if(fingers == 3):
                            response = requests.post(url, headers=headers, data=json.dumps(data1))
                            temp = fingers
                        elif(fingers == 4):
                            response = requests.post(url, headers=headers, data=json.dumps(data2))
                            temp = fingers
                        else:
                            continue

            cv2.imshow('Hand Finger Detection', frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    cap.release()
    cv2.destroyAllWindows()

if _name_ == "_main_":
    main()
