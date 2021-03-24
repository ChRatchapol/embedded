# SoupDoor
---

## อะไรคือ *SoupDoor* (วะ)?
---
__SoupDoor__ คือบอร์ด microcontroller รวมกับอุปกรณ์อิเล็กโทรนิกส์ ประตู และเลือดเนื้อของพวกเรา!

โอเค มันก็ไม่ขนาดนั้น __SoupDoor__ คือระบบล็อกไฟฟ้าที่ถูกออกแบบ และพัฒนาขึ้นมาเพื่อติดตั้งที่ห้องชุมนุมนิสิตวิศวกรรมคอมพิวเตอร์ มหาวิทยาลัยเกษตรศาสตร์ บางเขน ซึ่ง __SoupDoor__ จะเข้ามาช่วยให้การเข้าห้องชุมนุมฯ เป็นเรื่องที่ง่ายมากขึ้น และแน่นอนว่าปลอดภัยมากขึ้นนั่นเอง

## คุณควรรู้อะไรก่อนที่จะไปแตะต้องมัน
---
โอเค คือมันไม่ได้มีอะไรมากหรอก อย่างแรกคือคุณควรจะรู้ไว้ก่อนว่ามันเป็นระบบที่ค่อนข้างซับซ้อน ถึงมันจะไม่ไดู้ซับซ้อนมากมายนัก เพราะมันไม่ได้ออกแบบมาอย่างเรียบร้อยนัก เนื่องจากข้อจำกัดหลาย ๆ อย่าง เราพยายามอย่างหนักที่จะทำให้มันใช้งานได้ในทุก ๆ ช่วงเวลา

### ถ้าไฟตกละ
---
ถ้าคุณเคยใช้ หรือมีความรู้อยู่บ้างน่าจะรู้อยู่แล้วว่าล็อกไฟฟ้า...มันใช้ไฟฟ้า! น่าตกตะลึงใช่ไหมละ โอเค อาจจะไม่ขนาดนั้น แล้วจะเกิดอะไรขึ้นถ้าไฟตกละ อย่างแรกที่ต้องคิดถึงก่อนคือ เท่าที่รู้ และตั้งแต่อยู่ในห้องนั้นมา มันไม่เคยมีวันไหนไฟดับเลย อาจจะเพราะอยู่ในตึกภาคคอมพิวเตอร์ด้วย แต่เพื่อความปลอดภัย __SoupDoor__ ถูกออกแบบมาให้มีแหล่งพลังงานสำรองเป็นถ่านแบบชาร์จได้ 2 ชุดด้วยกัน โดยหากต้องการใช้ตอนไฟฟ้าดับจะต้องต่อแบตเตอร์รี่สำรองเข้าที่สายที่หน้าประตู เปิดสวิทช์ที่อยู่หน้าประตูเหมือนกัน และก็จะใช้งานได้ตามปกติ อย่าลืมถอดแบตเตอร์รี่สำรอง กับปิดสวิทช์ด้วยหลังเปิดได้ด้วยนะ เนื่องจากตัวบอร์ดเองค่อนข้างกินไฟเยอะ ดังนั้นอย่าลืมปิดสวิทช์ด้วย ไม่งั้นมันอาจจะเปิดไม่ได้อีกครั้ง เพราะถ่านหมดซะก่อน แน่นอนว่านอกจากตอนที่ไฟดับ และถ่านหมด __SoupDoor__ มันควรจะเปิดได้ตลอดเวลา

### ใช้ยังไงเนี่ย
---
อย่างแรกคือคุณต้องมีโทรศัพท์ซะก่อน เพราะการลงทะเบียนจะเริ่มใน LINE ของเรา เมื่อคุณกดลงทะเบียนจะมีรหัส OTP โผล่ขึ้นที่จอ OLED ที่หน้าห้องชุมนุมฯ หลังจากนั้นคุณมีเวลา 1 นาทีเพื่อกดรหัสที่ได้มาบนปุ่มที่อยู่หน้าประตู (เป็นปุ่มสี ก็กดตามสีเลย) หลังจากนั้นระบบจะถามว่าคุณมีบัตร RFID ไหม (จะขึ้นบนจอ) ให้กดปุ่มเขียวหากมี และกดปุ่มแดงหากไม่มี (แน่นอนว่าคุณมีเวลา 1 นาทีเพื่อกด) ถ้ามีบัตรหลังจากนั้นให้แตะบัตรที่เครื่องอ่าน เพื่อให้ระบบจำบัตรของคุณได้ (แน่นอนว่าภายใน 1 นาที) หลังจากนั้นเป็นอันเสร็จสิ้นการลงทะเบียน
วิธีการเปิดก็ง่ายแสนง่าย เพียงแค่คุณเอาบัตรแตะให้เครื่องอ่าน ถ้าบัตร และคุณได้ลงทะเบียนอย่างถูกต้อง ประตูก็จะเปิดออกทันที หรือถ้าคุณไม่มีบัตร หรือบัตรหาย คุณสามารถใช้ LINE ของเราเพื่อเปิดได้อีกด้วย โดยการกระทำทั้งหมดถูกบันึกเก็บไว้ใน Database ทั้งหมดเพื่อให้สามารถระบุตัวคนเข้าห้องได้นั้นเอง

ถ้าสงสัยละก็สามารถไปอ่านข้อมูลเพิ่มเติมได้ที่[ลิ้งก์นี้](https://docs.google.com/document/d/1m7Co31eBxPfTf1kkZ65ozW7nJEDyzLJ_TLQgXgb0yN0/edit?usp=sharing)

### ต้องดูแลยังไง
---
อย่างที่คุณได้อ่านมาระบบนี้ประกอบด้วยบอร์ดจำนวน 4 บอร์ด แบ่งเป็น 2 กลุ่มคือกลุ่มที่อยู่ที่หลังประตู กลุ่มนี้จะดูแลเรื่องปุ่มกด และล็อก โดยมีบอร์ดนึงเป็น Master ดูแลทั้งระบบ และมีอีกกลุ่มนึงอยู่บริเวณหน้าต่างคอยดูแลจอ เสียง และเครื่องอ่านบัตรนั้นเอง โดยบอร์ดทั้งหมดสื่อสารกัน และมีบอร์ด Master คอยคุยกับ Raspberry Pi ที่รัน Backend ไว้ ผ่านทาง I2C ที่เป็นสายเชื่อมข้ามมา (ดังนั้นหา Raspberry Pi มารันต่อด้วยถ้าเครื่องเก่ามีปัญหา หรือกำลังจะโดนเอาคืนไป ไม่งั้นจะเปิดด้วยไลน์ไม่ได้นะ)

ตัวระบบเองยังมีปุ่มใช้ในการเปิดประตูจากภายในโดยไม่ต้องอ่านบัตร หรือสั่งเปิด (ปุ่มนี้สั่งเขียนบัตรได้ด้วยนะ) และอีกปุ่มที่ใช้ในการลบข้อมูลบนบัตรได้ด้วย

ซึ่งจากที่บ่นมา จะเห็นได้ว่าถ้ามีบอร์ดใดบอร์ดหนึ่งมีปัญหาจะไม่สามารถใช้งานระบบได้อีกเลย โดยให้ใช้ Source Code ตัวนี้เพื่อ Reset ระบบกลับไปเป็นเวอร์่นเริ่มต้น (แน่ละ มันอาจจะไม่มีเวอร์ชั่นหลังจากนี้ก็ได้) และตัวถ่านเองเป็นถ่านชาร์จได้ ถ้าถ่านหมดก็สามารถชาร์จได้เลย อีกเรื่องที่ต้องดูแลดี ๆ คือเรื่องสายไฟ อย่าให้ขาดละ นอกนั้นก็เป็นการดูแลความสะอาดทั่ว ๆ ไปแล้ว

## โอเค แล้วถ้าไฟตก แล้วบัตรหายทำไงดี
---
จะมีบัตรอยู่ที่ห้องธุรการที่ตึกภาคฯ อยู่วิ่งขึ้นบันไดไปเอาซะ แต่ถ้าถ่านหมด ก็ปีนเอานะ!

## หากระบบนี้มีปัญหาใครคืคนแรกที่ต้องตามหา
---
แน่นอนว่าคือประธานชุมนุมฯ คนปัจจุบัน เพราะเขา/เธอจะได้รับการอธิบายการใช้งาน และการดูแล รวมไปถึงการแก้ปัญหาไว้อย่างละเอียดแล้วนั่นเอง

## ใครเป็นคนทำระบบนี้ขึ้นมา
---
พวกเราเป็นหนึ่งในทีมทำงานของชุมนุมนิสิตวิศวกรรมคอมพิวเตอร์ ปีการศึกษา 2563 โดยเราทำงานนี้เป็นโปรเจคของวิชา Embedded System จึงนำระบบนี้มาติดตั้งเพื่อใช้จริงด้วย ซึ่งถ้าหากมีปัญหา หรือต้องการปรึกษา สามารถติดต่อเราได้ตามด้านล่าง

| ชื่อ-นามสกุล | Facebook | โทรศัพท์ |
|------------------|----------------------------------------------------------------------|--------------------|
| นายรัชพล จันทรโชติ | FB: [Ratchapol Chantarachote](https://www.facebook.com/ch.ratchapol) | 0817126100 |
| นายอิทธิกร ปุญสิริ | FB: [Aitthikorn Poonsiri](https://www.facebook.com/gorn.aitthikorn/) | 0918549003 |        
| นายนนท์ วิทวัสการเวช | FB: [นนท์ไง](https://www.facebook.com/Non.Nontosan) | 0890888168 |
| นายภัฎสร์ชนน ศรีรักษา | FB: [Aom Patschanon Sriraksa](https://www.facebook.com/AomPS.SKR.20TH) | 0970616136 |        

---

นายรัชพล จันทรโชติ    
ประธานชุมนุมนิสิตวิศวกรรมคอมพิวเตอร์ ปีการศึกษา 2563    
เขียน ณ วันที่ 17 มีนาคม พ.ศ. 2564
