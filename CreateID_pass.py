#!/usr/bin/env python
# -*- coding: utf-8 -*-

#*
#* CreateID_pass.py
#*
#* Copyright (c) 2020 IWATA Daiki
#*
#* This software is released under the MIT License.
#* see http://opensource.org/licenses/mit-license
#*


import sys
import hashlib
import sqlite3


userID = input("Input userID:   ")
passwd = input("Input password: ")



while True:
    check = input("Are you sure to register this userID and password[y/n]?: ")

    if(check == "y"):
        break;
    elif(check == "n"):
        sys.exit();
    else:
        print("Please input [y] or [n].")
    
    
pwhash = hashlib.sha256(passwd.encode('utf-8')).hexdigest()
# print(pwhash)

conn = sqlite3.connect("testdb")
conn.execute("insert into users values( ?,? )", [userID, pwhash])

conn.commit()
