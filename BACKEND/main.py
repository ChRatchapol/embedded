from flask import Flask, request, abort
from flask_pymongo import PyMongo
import datetime
import random
import json
import pytz
import os

# Import Linebot
from linebot import (LineBotApi, WebhookHandler)
from linebot.exceptions import (InvalidSignatureError)
from linebot.models import (MessageEvent, TextMessage, TextSendMessage,
                            ConfirmTemplate, MessageAction, TemplateSendMessage)

# Import Config
import configparser
config = configparser.ConfigParser()
config.read('config.ini')
MONGO_URI = config['MONGODB']['MONGO_URI']
CHANNEL_SECRET = config['LINE_BOT']['CHANNEL_SECRET']
CHANNEL_ACCESS_TOKEN = config['LINE_BOT']['CHANNEL_ACCESS_TOKEN']

# Initialize
app = Flask(__name__)
app.config['MONGO_URI'] = MONGO_URI
mongo = PyMongo(app)
line_bot_api = LineBotApi(CHANNEL_ACCESS_TOKEN)
handler = WebhookHandler(CHANNEL_SECRET)
rfidLogsCollection = mongo.db.rfidLogs
lineLogsCollection = mongo.db.lineLogs
buttonLogsCollection = mongo.db.buttonLogs
usersCollection = mongo.db.users
SIGNUP_SESSION = ()


# @app.route('/unlock', methods=['GET'])
def unlockDoor(**kwargs):
    userId = kwargs['userId']
    replyToken = kwargs['replyToken']
    filt = {'userId': userId}

    # if permission granted -> request device to unlock door
    if (usersCollection.find_one(filt)):

        # Send message to Pi
        os.system('printf "{type: 3, by: 1, uuid: \'' + userId + '\'}" > message.json')

        return 'OK', 200

    else:
        line_bot_api.reply_message(replyToken, TextSendMessage(
            text='กรุณาลงทะเบียนก่อนใช้งานนะ'))
        return 'Unauthorized', 401

@app.route('/unlock/validate', methods=['POST'])
def unlockValidate():
    data = request.json
    try:
        rfid = data['uuid']
    except:
        return 'Bad Request', 400
    
    filt = {'rfid': rfid}
    if (usersCollection.find_one(filt)):
        os.system('printf "{type: 3, by: 0, uuid: \'' + rfid + '\'}" > message.json')
        return 'OK', 200

    else: 
        os.system('printf "{type: 5}" > message.json')
        return 'Unauthorized', 401


@app.route('/logging', methods=['POST'])
def logging():
    data = request.json
    try:
        unlockMethod = data['from']
        uuid = data['uuid']  # uuid or empty string
    except:
        return 'Bad Request', 400

    # Line Log
    if (unlockMethod == 'LINE'):
        # query username matched this filter
        user = usersCollection.find_one({'userId': uuid})
        userId = user['userId']
        username = user['username']

        # logging
        ts = datetime.datetime.now(pytz.timezone('Asia/Bangkok'))
        lineLogsCollection.insert_one({
            'userId': userId,
            'username': username,
            'timestamp': ts.strftime("%H:%M:%S - %d/%b/%Y")
        })

        # reply message
        line_bot_api.push_message(
            userId, TextSendMessage(text='เปิดประตูละจ้า'))

    # Button Log
    elif (unlockMethod == 'Bttn'):
        ts = datetime.datetime.now(pytz.timezone('Asia/Bangkok'))
        buttonLogsCollection.insert_one({
            'timestamp': ts.strftime("%H:%M:%S - %d/%b/%Y")
        })

    # RFID Log
    else:
        ts = datetime.datetime.now(pytz.timezone('Asia/Bangkok'))
        rfidLogsCollection.insert_one({
            'rfid': uuid,  # uuid or empty string
            'timestamp': ts.strftime("%H:%M:%S - %d/%b/%Y")
        })

    return 'OK', 200


@app.route('/signup/validate', methods=['POST'])
def signupValidate():
    global SIGNUP_SESSION
    data = request.json

    try:
        status = data['type']
    except:
        return 'Bad Request', 400

    try:
        userId = SIGNUP_SESSION[1]['userId']
        username = SIGNUP_SESSION[1]['username']
    except:
        return 'Not Acceptable', 406

    if status == 255:  # register failed
        # Clear Session
        SIGNUP_SESSION = {}

        line_bot_api.push_message(userId, TextSendMessage(
            text='การลงทะเบียนไม่สำเร็จ กรุณาทำการกดลงทะเบียนใหม่หากต้องการใช้งาน'))
        return 'Bad Request', 400

    elif status == 1:  # OTP Validate ?
        if (SIGNUP_SESSION[1]['status'] == 'processing rfid'):
            try:
                rfid = data['rfid']
            except:
                return 'Bad Request', 400

            # Clear Session
            SIGNUP_SESSION = {}
            app.logger.info(SIGNUP_SESSION)
            usersCollection.insert_one({
                'userId': userId,
                'username': username,
                'rfid': rfid
            })
            
            line_bot_api.push_message(userId, TextSendMessage(
                text='ลงทะเบียนบัตร RFID เรียบร้อย'))
        else:
            # OTP check twice!
            try:
                key = data['key']
            except:
                return 'Bad Request', 400

            for i in range(len(SIGNUP_SESSION[0])):
                if (key[i] != SIGNUP_SESSION[0][i]):
                    # Clear Session
                    SIGNUP_SESSION = {}

                    line_bot_api.push_message(userId, TextSendMessage(
                        text='การลงทะเบียนไม่สำเร็จ กรุณาทำการกดลงทะเบียนใหม่หากต้องการใช้งาน'))
                    os.system('printf "{type: 255}" > message.json')

                    return 'Bad Request', 400

            rfid_confirm = ConfirmTemplate(text='ลงทะเบียนเสร็จสิ้น ต้องการลงทะเบียนบัตร RFID ด้วยหรือไม่', actions=[
                MessageAction(label='ใช่', text='ใช่'),
                MessageAction(label='ไม่', text='ไม่')
            ])

            SIGNUP_SESSION[1]['status'] = 'waiting for rfid'
            
            line_bot_api.push_message(
                userId, TemplateSendMessage(
                    alt_text='ลงทะเบียนบัญชีผู้ใช้เสร็จสิ้น ต้องการลงทะเบียนบัตร RFID ด้วยหรือไม่',
                    template=rfid_confirm
                )
            )

    return 'OK', 200


@app.route('/signup/request', methods=['POST'])
def signupKeygen(**kwargs):
    global SIGNUP_SESSION

    if (usersCollection.find_one({"userId": kwargs['userId']})):
        line_bot_api.reply_message(kwargs['replyToken'], TextSendMessage(
            text='คุณได้ทำการลงทะเบียนไปเรียบร้อยแล้ว'))

    else:
        key = ''.join([random.choice(["R", "G", "B", "Y"]) for _ in range(8)])
        SIGNUP_SESSION = (key, {'userId': kwargs['userId'],
                                'username': '',
                                'rfid': '',
                                'status': 'waiting for username'})
        line_bot_api.reply_message(kwargs['replyToken'], TextSendMessage(
            text='กรุณาพิมพ์ username ที่ต้องการใช้'))


def signupSession(text, replyToken, userId):
    global SIGNUP_SESSION
    # Check if user is same with session owner
    if (SIGNUP_SESSION[1]['userId'] == userId):
        if (text == 'ยกเลิก'):
            # Clear Session
            SIGNUP_SESSION = {}
            os.system('printf "{type: 255}" > message.json')
            return line_bot_api.reply_message(replyToken, TextSendMessage(text='ยกเลิกการทำรายการเรียบร้อย'))

        if (SIGNUP_SESSION[1]['status'] == 'waiting for username'):
            if (text == 'Register'):
                return signupKeygen(replyToken=replyToken, userId=userId)
            SIGNUP_SESSION[1]['username'] = text
            SIGNUP_SESSION[1]['status'] = 'processing otp'

            # Send message to Pi
            os.system('printf "{type: 1, OTP: \'' +
                      SIGNUP_SESSION[0] + '\'}" > message.json')

            return line_bot_api.reply_message(replyToken, TextSendMessage(text=f'คุณ {text}\nสิ้นสุดขั้นตอนการตั้ง username\nกรุณากดรหัสตามที่แสดงบนหน้าจอ OLED\nหากต้องการยกเลิกหรือต้องการกดใหม่ให้พิมพ์คำว่า "ยกเลิก"'))

        elif (SIGNUP_SESSION[1]['status'] == 'processing otp'):
            return line_bot_api.reply_message(replyToken, TextSendMessage(text='กรุณากดรหัสตามที่แสดงบนหน้าจอ OLED\nหากต้องการยกเลิกหรือต้องการกดใหม่ให้พิมพ์คำว่า "ยกเลิก"'))

        elif (SIGNUP_SESSION[1]['status'] == 'waiting for rfid'):
            if (text == 'ใช่'):
                SIGNUP_SESSION[1]['status'] = 'processing rfid'
                
                # Send message to Pi
                os.system('printf "{type: 4}" > message.json')

                return line_bot_api.reply_message(replyToken, TextSendMessage(text='เริ่มต้นขั้นตอนการลงทะเบียนบัตร RFID\nกรุณาแตะบัตร RFID')) 

            elif (text == 'ไม่'):
                # Clear Session
                SIGNUP_SESSION = {}

                usersCollection.insert_one({
                    'userId': userId,
                    'username': SIGNUP_SESSION[1]['username'],
                    'rfid': None
                })
                return line_bot_api.reply_message(replyToken, TextSendMessage(text='จบขั้นตอนการลงทะเบียนบัตร RFID')) 

            else:
                return line_bot_api.reply_message(replyToken, TextSendMessage(text='กรุณากดเลือก "ใช่" หรือ "ไม่" เท่านั้น')) 

    else:
        return line_bot_api.reply_message(replyToken, TextSendMessage(
            text='ขณะนี้มีผู้ใช้งานท่านอื่นกำลังทำการลงทะเบียนอยู่'))


#   _     _              ____        _        _    ____ ___
#  | |   (_)_ __   ___  | __ )  ___ | |_     / \  |  _ \_ _|
#  | |   | | '_ \ / _ \ |  _ \ / _ \| __|   / _ \ | |_) | |
#  | |___| | | | |  __/ | |_) | (_) | |_   / ___ \|  __/| |
#  |_____|_|_| |_|\___| |____/ \___/ \__| /_/   \_\_|  |___|


@app.route('/webhook', methods=['GET', 'POST'])
def webhook():
    # get X-Line-Signature header value
    signature = request.headers['X-Line-Signature']

    # get request body as text
    body = request.get_data(as_text=True)
    app.logger.info("Request body: " + body)
    # app.logger.info(SIGNUP_SESSION)

    # handle webhook body
    try:
        handler.handle(body, signature)
    except InvalidSignatureError:
        abort(400)

    return 'OK', 200


@handler.add(MessageEvent, message=TextMessage)
def handle_message(event):
    global SIGNUP_SESSION

    text = event.message.text
    userId = event.source.user_id
    replyToken = event.reply_token
    # displayName = line_bot_api.get_profile(userId).display_name

    # Check if have a session.
    if (SIGNUP_SESSION):
        return signupSession(text=text, replyToken=replyToken, userId=userId)

    # Rich Menu Switcher
    if (text == 'Open The Door'):
        return unlockDoor(replyToken=replyToken, userId=userId)
    elif (text == 'Register'):
        return signupKeygen(replyToken=replyToken, userId=userId)
    elif (text == 'Contact Admin'):
        pass


if __name__ == "__main__":
    app.run(host="0.0.0.0", port="7002")
