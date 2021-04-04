# SoupDoor
---
ระบบล็อกไฟฟ้าที่ถูกออกแบบ และพัฒนาขึ้นมาเพื่อติดตั้งที่ห้องชุมนุมนิสิตวิศวกรรมคอมพิวเตอร์ มหาวิทยาลัยเกษตรศาสตร์ บางเขน 

### Document
---
สำหรับดีเทลของการทำงานและวิธีใช้งานโดยละเอียด : [DOCS](https://docs.google.com/document/d/1m7Co31eBxPfTf1kkZ65ozW7nJEDyzLJ_TLQgXgb0yN0/edit?usp=sharing)

## ไฟล์ที่เกี่ยวข้อง
---
ประกอบด้วย 5 folder คือ    

1.BACKEND เป็น folder ที่เก็บ source code สำหรับ backend โดยมี 2 ไฟล์ด้วยกัน คือ i2c_master.py คือไฟล์ที่ประกอบด้วยโค้ดสำหรับ รับและส่ง i2c ระหว่าง board ESP32 หลัก และ Raspberry Pi และอีกไฟล์คือ main.py ที่ทำการรัน flask ไว้สำหรับส่งข้อมูลไปที่ ฐานข้อมูล และมี line bot ที่ใช้ทำงานนี้ โดยทั้งสองไฟล์(i2c_master.py และ main.py)รันอยู่บน raspberry pi          
2.MASTER เป็น folder เก็บ source code ของบอร์ด ESP32 ตัวหลักเอาไว้หรือเรียนว่า บอร์ด MASTER จะเป็นบอร์ดที่ติดต่อกับ Raspberry Pi และเป็น MASTER รอรับข้อมูล ส่งให้ pi          
3.BUTTON เป็น folder เก็บ source code ของบอร์ด ESP32 ที่ควบคุมปุ่มกด 4 สี รอรับรหัส OTP จาก MASTER และส่งกลับสถานะความถูกต้องกลับไป            
4.OLED_BUZZER เป็น folder เก็บ source code ของบอร์ด ESP32 ที่ควบคุม จอ OLED แสดงผลต่างๆ และ เสียง BUZZER ที่บอกสถานะการทำงานต่างๆด้วย                                   
5.RFID เป็น folder เก็บ source code ของบอร์ด ESP32 ที่ควบคุม อุปกรณ์อ่าน/เขียน RFID และคอยส่งข้อมูลบัตรให้กับ MASTER                                      

## 

## ผู้จัดทำ
---
| ชื่อ-นามสกุล | Facebook | โทรศัพท์ | Student ID |
|------------------|----------------------------------------------------------------------|--------------------|------------|
| นายรัชพล จันทรโชติ | FB: [Ratchapol Chantarachote](https://www.facebook.com/ch.ratchapol) | 0817126100 | 6110500143 |
| นายอิทธิกร ปุญสิริ | FB: [Aitthikorn Poonsiri](https://www.facebook.com/gorn.aitthikorn/) | 0918549003 | 6110500640 |       
| นายนนท์ วิทวัสการเวช | FB: [นนท์ไง](https://www.facebook.com/Non.Nontosan) | 0890888168 | 6110503312 |
| นายภัฎสร์ชนน ศรีรักษา | FB: [Aom Patschanon Sriraksa](https://www.facebook.com/AomPS.SKR.20TH) | 0970616136 | 6110503401 |

---

ภาควิชาวิศวกรรมคอมพิวเตอร์ คณะวิศวกรรมศาสตร์ มหาวิทยาลัยเกษตรศาสตร์

## License
---

MIT License
