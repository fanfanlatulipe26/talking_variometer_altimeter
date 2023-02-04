# Script Python 
# Generation of the mp3 files for numbers and messages for the talking variometer+altimeter
# For DFPlayer optimisation we have:
# One mp3 file for each number from 0 to 999, stored in several folders 01, 02, 03, 04
# Each folder contains 250 files, named 001.mp3, 002.mp3  ...
# Filename 000 is illegal in DFPlayer and so sound for number 0 is in file 001.mp3 in the first folder 01
# In folder 00 fil 001.mp3 if for nmber 0
# In folder 02  file 001.mp3 is for number 250 ....

# Folder 05 contains the thousands from 1000 (file 001.mp3) to 10000 (file 010.mp3)
# Folder 06 contains the messages (see file messages.txt )
# 

langage = 'it'     # change this line for support of a new language. 

topLevelDomain = 'com'  # change this line if you want a specific Top Level Domain.
#

from gtts import gTTS
#from pydub import AudioSegment
import os
import shutil
import eyed3
import csv

filePerDir = 250
dirMp3= 'mp3'

curDir=''

shutil.rmtree(dirMp3,ignore_errors=True)
os.makedirs(dirMp3,exist_ok=True)

for n in range(0,1000):
  tts = gTTS(str(n), lang=langage, tld= topLevelDomain )
  if(str(int(n/filePerDir)+1).zfill(2) != curDir ):
    curDir = str(int(n/filePerDir)+1).zfill(2)
    os.makedirs(dirMp3 + '/' + curDir,exist_ok=True)
  filename = str(n%filePerDir+1).zfill(3)
  print(n,  curDir ,  filename)
  fullPath = dirMp3 + '/' + curDir + '/' + filename
  tts.save(fullPath + '.mp3')
  audiofile = eyed3.load (fullPath + '.mp3') #Load the file
  audiofile.initTag()
  audiofile.tag.title = str(n)
  audiofile.tag.save()
 
curDir = str(int(1000/filePerDir)+1).zfill(2)  # folder for "thousands"
os.makedirs(dirMp3 + '/' + curDir,exist_ok=True)
for n in range(1000,11000,1000):
  tts = gTTS(str(n), lang=langage, tld= topLevelDomain )
  filename = str(int(n/1000)).zfill(3)
  print(n,  curDir ,  filename)
  fullPath = dirMp3 + '/' + curDir + '/' + filename
  tts.save(fullPath + '.mp3')
  audiofile = eyed3.load(fullPath + '.mp3') #Load the file
  audiofile.initTag()
  audiofile.tag.title = str(n)
  audiofile.tag.save()

curDir = str(int(1000/filePerDir)+2).zfill(2)  # folder for "messages"
os.makedirs(dirMp3 + '/' + curDir,exist_ok=True)
print("Folder for messages",curDir)
with open('messages.txt', encoding='utf8') as csvfile:
         audioreader = csv.reader(csvfile, delimiter=';')
         next(audioreader)
         for row in audioreader:
             if row == []:
                 continue
             name = row[0]
             text = row[1]
             filename = str(name).zfill(3) 
             tts = gTTS(text, lang=langage, tld= topLevelDomain )
             fullPath = dirMp3 + '/' + curDir + '/' + filename      
             tts.save(fullPath + '.mp3')
             print(filename, text)
             audiofile = eyed3.load(fullPath + '.mp3') #Load the file
             audiofile.initTag()
             audiofile.tag.title = text
             audiofile.tag.save()
 