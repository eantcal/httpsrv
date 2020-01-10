#!/bin/bash 

echo -----------------------
echo httpsrv functional test
echo -----------------------

# Server is expected to run with the following configuration
host_and_port="localhost:8080"
working_dir="$HOME/.httpsrv"

# ------------------------------------------------------------------------------
# Utils
# ------------------------------------------------------------------------------

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

checkCommand() {
  commandName=$1
  if ! [ -x "$(command -v $commandName)" ]; then
    echo 'Error: $commandName is not installed.' >&2
    exit 1
  fi
}


cleanUpFile () {
  fileToClean=$1

  sed -i "s/\"//g" $fileToClean
  sed -i "s/,//g" $fileToClean
  sed -i '/^$/d' $fileToClean
}

fail() {
  failMsg=$1
   
  set +x
  printf "[$RED Error $NC] $failMsg" >&2
  exit 1
}

success() {
  timestamp=`date`
  echo "[ ${GREEN}OK${NC} $timestamp] $1" >> $resultfile
}


# ------------------------------------------------------------------------------
# Checks needed tools are there 
# ------------------------------------------------------------------------------

checkCommand "curl"
checkCommand "sed"
checkCommand "awk"
checkCommand "unzip"
checkCommand "sha256sum"

jsonvalidator="jsonlint-php"
checkCommand $jsonvalidator

#set -x

tmp_dir=$(mktemp -d -t resources-XXXXXXXXXX)
tmp_dir2=$(mktemp -d -t resources-XXXXXXXXXX)
rm -f $HOME/.httpsrv/File*.txt

if [ -d "$tmp_dir" ]; then
  echo "Created ${tmp_dir}..."
else
  echo "Error: ${tmp_dir} not found. Can not continue." >&2
  exit 1
fi

listfile=$tmp_dir"/list.tmp"
resultfile=$tmp_dir"/result.tmp"
allidsfile=$tmp_dir"/allids.tmp"
mruidsfile=$tmp_dir"/mruids.tmp"
threeidsfile=$tmp_dir"/threeids.tmp"
sortedmruidsfile=$tmp_dir"/sortedmruids.tmp"
NUMOFFILES=50

#Create a bunch of text files with different growing sizes
CONTENT=""
echo "Creating $NUMOFFILES files in $tmp_dir ..."
for i in `seq -f "%02g" 1 $NUMOFFILES`
do
    ADDCONTENT=" Content"
    CONTENT+=$ADDCONTENT
    echo $CONTENT
    echo $CONTENT > "$tmp_dir/File${i}.txt"
done

rm -f $resultfile

# ------------------------------------------------------------------------------
# Upload $NUMOFFILES files via http onto remote repository
# ------------------------------------------------------------------------------

listcontent=`find $tmp_dir -type f -name *.txt -exec curl -F file=@{} $host_and_port/store \;`

echo $listcontent | sed -r "s/\}/\\n/g" > $listfile

chk=`cat $listfile | grep id | grep name | grep timestamp | wc -l`

if [ $chk = "$NUMOFFILES" ]; then
  success "POST /store: all $NUMOFFILES files uploaded without errors"
else
  fail "POST /store: Expected $NUMOFFILES notifications\n" 
fi

cleanUpFile $listfile

# ------------------------------------------------------------------------------
# Get the list of ids in the local temporary dir
# ------------------------------------------------------------------------------

rm -f $allidsfile

echo "Searching for 3 most recent files in $tmp_dir"
while read p; do
  ok=0
  curl $host_and_port/files/`echo "$p" | awk '{print $3}';` | grep id | awk '{print $2}' >> $allidsfile && ok=1

  id=`echo $p | awk '{print $3 " for " $5}';`
  [[ ! -z "$first_id" ]] && [[ -z "$second_id" ]] && second_id=`echo $p | awk '{print $3}'`
  [[ -z "$first_id" ]] && first_id=`echo $p | awk '{print $3}'`

  if [ $ok = "1" ]; then
    success "GET /files/$id: succeded"
  else
    fail "GET /files/<${id}>: can't get a correct answer\n"
  fi
done < $listfile 

cleanUpFile $allidsfile

# ------------------------------------------------------------------------------
# Select 3 mru from local repository
# ------------------------------------------------------------------------------

tail -n 3 $allidsfile | sort > $threeidsfile

echo "Getting the mrufils from http server" 
rm -f $mruidsfile


# ------------------------------------------------------------------------------
# Query the server for getting the mrufiles and check if the content matches
# with 3 mru repository files
# ------------------------------------------------------------------------------

curl $host_and_port/mrufiles | grep id >> $mruidsfile
cleanUpFile $mruidsfile

cp -f $mruidsfile $sortedmruidsfile
cat $sortedmruidsfile | awk '{print $2}' > $mruidsfile
cat $mruidsfile | sort > $sortedmruidsfile

ok=0
diff $threeidsfile $sortedmruidsfile && ok=1

if [ $ok = "1" ]; then
  success "GET /mrufiles: Get OK, content validated" 
else
  fail "GET /mrufiles: response and actual f/s files look different\n"
fi


# ------------------------------------------------------------------------------
# Get a single zip file
# ------------------------------------------------------------------------------
ok=0
curl --output $tmp_dir2/$first_id.zip $host_and_port/files/$first_id/zip && ok=1 

if [ $ok = "1" ]; then
  success "GET /files/$first_id/zip: zip file downloaded" 
else
  fail "GET /files/$first_id/zip: Can't download the zip file\n"
fi

filename=`zipinfo $tmp_dir2/$first_id.zip | grep File | awk '{ print $9 }'`
ok=0
cd $tmp_dir2 && unzip $tmp_dir2/$first_id.zip && cd - && ok=1

if [ $ok = "0" ]; then
  fail "GET /files/$first_id/zip: failed downloading the zip file\n"
fi

diff $tmp_dir2/$filename $tmp_dir/$filename || ok=0
if [ $ok = "0" ]; then
  fail "GET /files/$first_id/zip: unexpected zip archive content\n"
fi


# ------------------------------------------------------------------------------
# Get mrufiles zip file
# ------------------------------------------------------------------------------
ok=0
rm -f $tmp_dir2/*
curl --output $tmp_dir2/mrufiles.zip $host_and_port/mrufiles/zip && ok=1 

if [ $ok = "0" ]; then
  fail "GET /mrufiles/zip: Can't download the mrufiles archive\n"
fi

ok=0
cd $tmp_dir2 && unzip ./mrufiles.zip && rm mrufiles.zip && cd - && ok=1

if [ $ok = "0" ]; then
  cd -
  fail "GET /mrufiles/zip: Failed uncompress the mrufiles.zip\n"
fi

ok=0
cd $tmp_dir2
ls `curl $host_and_port/mrufiles | grep File | sed "s/\"//g" | sed "s/,//g" | awk '{print $2}'` && ok=1
filescount=`ls *.txt | wc -w`
cd -

if [ $ok = "1" ] && [ $filescount = "3" ]; then
  success "GET /mrufiles/zip: mrufiles entry count verified"
else
  fail "GET /mrufiles/zip: Unexpected number of entries in mrufiles.zip archive\n"
fi

# ------------------------------------------------------------------------------
# Verify the JSON validity of JSON httpsrv resposese to fils and mrufils 
# command 
# ------------------------------------------------------------------------------

function getRequest() {
  getRequestType=$1

  ok=0
  curl $host_and_port/$getRequestType > $tmp_dir/$getRequestType.json && ok=1
  if [ $ok = "0" ]; then
    fail "GET /$getRequestType: Can't get an answer\n"
  fi

  ok=0
  eval "$jsonvalidator $tmp_dir/$getRequestType.json && ok=1" 2>/dev/null 
  if [ $ok = "0" ]; then
    fail "GET /$getRequestType: Can't validate JSON\n"
  fi

  success "GET /$getRequestType: JSON Valid"
}

getRequest files
getRequest mrufiles

# ------------------------------------------------------------------------------
# TIMESTAMP validations
# ------------------------------------------------------------------------------

#GET /file/<id>/zip
#check that the first_id related entry now is into mrufiles list
#got in a previous GET /file/<id>/id is part of mrufiles.json

sleep 1.5
touch $working_dir/File01.txt
touch $working_dir/File02.txt
touch $working_dir/File03.txt
sync

id=`echo -n File04.txt | sha256sum  | awk '{print $1}'`

ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null || ok=1
if [ $ok = "0" ]; then
   fail "GET /files/$first_id: %first_id should not be in the mrulist now\n"
fi

# make sure timestamp distance at least is 1 sec
sleep 1.5

ok=0
curl --output $tmp_dir2/$id.zip $host_and_port/files/$id/zip && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$id: Can't download zip file\n" 
fi

ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$id: Can't get the mru list\n"
fi

success "GET /files/$id/zip: Timestamp updated correctly"

#GET /file/<id>
#check that the first_id related entry now is into mrufiles list

id=`echo -n File05.txt | sha256sum  | awk '{print $1}'`

ok=1
curl $host_and_port/mrufiles | grep $id && ok=0
 if [ $ok = "0" ]; then
   fail "GET /files/$id: the id is not supposed to be in the MRU list\n"
fi

# make sure timestamp distance at least is 1 sec
sleep 1.5

ok=0
curl $host_and_port/files/$id > $tmp_dir/aFile.json && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$second_id: Can't get the file descriptor\n"
fi

ok=0
eval "$jsonvalidator $tmp_dir/aFile.json && ok=1" 2>/dev/null 
if [ $ok = "0" ]; then
  fail "GET /files/$id : Can't validate the JSON output\n"
else
  success "GET /files/$id: JSON response for single id is Valid"
fi

ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$second_id: Can't get the mru list\n"
fi


success "GET /files/$second_id: timestamp looks OK"

# ------------------------------------------------------------------------------
# Big File Test
# ------------------------------------------------------------------------------

bigFileSrc="/usr/share/dict/words"
bigFileName="bigFile.txt"

cp -f $bigFileSrc $tmp_dir/$bigFileName

bigfileid=`echo -n ${bigFileName} | sha256sum  | awk '{print $1}'`

ok=0
cd $tmp_dir && curl -F file=@${bigFileName} $host_and_port/store > $bigFileName.json && cd - && ok=1
if [ $ok = "0" ]; then
  fail "POST $bigFileName/store: Cannot transfer ${tmp_dir}/${bigFileName}\n"
fi

ok=0
grep $bigfileid $tmp_dir/$bigFileName.json && ok=1
if [ $ok = "0" ]; then
  fail "POST $bigFileName/store: Invalid response for ${tmp_dir}/${bigFileName}\n"
fi

success "POST store/$bigFileName: the 'big file' has been uploaded correctly"


# ------------------------------------------------------------------------------
# Zero File Test
# ------------------------------------------------------------------------------

zeroFileName="zeroFile.txt"

touch $tmp_dir/$zeroFileName

zerofileid=`echo -n ${zeroFileName} | sha256sum  | awk '{print $1}'`

ok=0
cd $tmp_dir && curl -F file=@${zeroFileName} $host_and_port/store > $zeroFileName.json && cd - && ok=1
if [ $ok = "0" ]; then
  fail "POST $zeroFileName/store: Cannot transfer ${tmp_dir}/${zeroFileName}\n"
fi

ok=0
grep $zerofileid $tmp_dir/$zeroFileName.json && grep '"size": 0,' $tmp_dir/$zeroFileName.json && ok=1
if [ $ok = "0" ]; then
  fail "POST $zeroFileName/store: Invalid response for ${tmp_dir}/${zeroFileName}\n"
fi

rm -f $tmp_dir/$zeroFileName

ok=0
curl --output $tmp_dir/$zerofileid.zip $host_and_port/files/$zerofileid/zip && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$id/zip: Zero size file download failed\n" 
fi

ok=0
cd $tmp_dir && unzip $tmp_dir/$zerofileid.zip && cd - && ok=1
if [ $ok = "0" ]; then
  fail "GET /files/$id/zip: Zip file not found\n" 
fi

FILESIZE=$(stat -c%s "$zeroFileName")
if [ $FILESIZE -ne "0" ]; then
  fail "GET /files/$id/zip: Zip file size $FILESIZE != 0\n" 
fi

success "POST store/$zeroFileName: zero-size file is handled correctly"

# ------------------------------------------------------------------------------
# Evil Requests
# ------------------------------------------------------------------------------

sendWrongRequest() {
  uriToSend=$1
  errorExpected=$2

  ok=0
  curl $host_and_port/$uriToSend > $tmp_dir/err.html && ok=1

  if [ $ok = "0" ]; then
    fail "GET $uriToSend: Error in Server Response\n"
  fi

  ok=0
  grep "$errorExpected" $tmp_dir/err.html && ok=1

  if [ $ok = "0" ]; then
    fail "GET $uriToSend: Wrong error message detected\n"
  fi

  success "GET $uriToSend: expected HTTP error checked"
}

sendWrongRequest mrufiles/ThisIsInvalid     "400 Bad Request"
sendWrongRequest files/InvalidID            "404 Not Found"
sendWrongRequest files/invalidID/zip        "404 Not Found" 
sendWrongRequest mrufiles/zip/thisIsInvalid "400 Bad Request"

set +x

# ------------------------------------------------------------------------------
# Show results
# ------------------------------------------------------------------------------
clear
echo ------------------------------------
printf "${GREEN}TEST SUCCEDED${NC}\n"
echo ------------------------------------
while read -r line; do echo -e $line; done < $resultfile


# ------------------------------------------------------------------------------
# Cleanup temporary directories and related files
# ------------------------------------------------------------------------------
rm -rf $tmp_dir
rm -rf $tmp_dir2
