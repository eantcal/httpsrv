#!/bin/bash 

echo -----------------------
echo httpsrv functional test
echo -----------------------

# Server is expected to run with the following configuration
host_and_port="localhost:8080"
working_dir="$HOME/.httpsrv"

# ------------------------------------------------------------------------------
# Init
# ------------------------------------------------------------------------------


if ! [ -x "$(command -v curl)" ]; then
  echo 'Error: curl is not installed.' >&2
  exit 1
fi

if ! [ -x "$(command -v sed)" ]; then
  echo 'Error: sed is not installed.' >&2
  exit 1
fi


if ! [ -x "$(command -v awk)" ]; then
  echo 'Error: awk is not installed.' >&2
  exit 1
fi

if ! [ -x "$(command -v unzip)" ]; then
  echo 'Error: unzip is not installed.' >&2
  exit 1
fi

jsonvalidator="jsonlint-php"
if ! [ -x "$(command -v ${jsonvalidator})" ]; then
  echo 'Error: ' ${jsonvalidator} ' is not installed.' >&2
  exit 1
fi

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

set -x

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

#Create a bunch of text files with different growing sizes
CONTENT=""
echo "Creating 10 files in $tmp_dir ..."
for i in {01..10}
do
    ADDCONTENT=" Content"
    CONTENT+=$ADDCONTENT
    echo $CONTENT
    echo $CONTENT > "$tmp_dir/File${i}.txt"
done

rm -f $resultfile

# ------------------------------------------------------------------------------
# Uploading all the files via http
# ------------------------------------------------------------------------------

echo "Upload files into store via the http server"
listcontent=`find $tmp_dir -type f -name *.txt -exec curl -F name=@{} $host_and_port/store \;`

echo $listcontent | sed -r "s/\}/\\n/g" > $listfile

chk=`cat $listfile | grep id | grep name | grep timestamp | wc -l`

if [ $chk = "10" ]; then
  echo "OK    POST /store" >> $resultfile
else
  set +x
  printf "POST /store ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

sed -i "s/\"//g" $listfile
sed -i "s/,//g" $listfile
sed -i '/^$/d' $listfile

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
    echo "OK    GET /files/$id" >> $resultfile
  else
    set +x
    printf "GET /files/<${id}> ${RED}TEST FAILED${NC}\n" >&2
    exit 1
  fi
done < $listfile 

sed -i "s/\"//g" $allidsfile
sed -i "s/,//g" $allidsfile
sed -i '/^$/d' $allidsfile

# ------------------------------------------------------------------------------
# Select 3 mru from local repository
# ------------------------------------------------------------------------------

tail -n 3 $allidsfile | sort > $threeidsfile

echo "Getting the mrufils from http server" 
rm -f $mruidsfile


# ------------------------------------------------------------------------------
# Query the server for getting the mrufiles
# ------------------------------------------------------------------------------

curl $host_and_port/mrufiles | grep id >> $mruidsfile
sed -i "s/\"//g" $mruidsfile
sed -i "s/,//g" $mruidsfile
sed -i '/^$/d' $mruidsfile
 
cp $mruidsfile $sortedmruidsfile
cat $sortedmruidsfile | awk '{print $2}' > $mruidsfile
cat $mruidsfile | sort > $sortedmruidsfile

ok=0
diff $threeidsfile $sortedmruidsfile && ok=1

if [ $ok = "1" ]; then
  echo "OK    GET /mrufiles" >> $resultfile
else
  set +x
  printf "GET /mrufiles ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi


# ------------------------------------------------------------------------------
# Get a single zip file
# ------------------------------------------------------------------------------
ok=0
curl --output $tmp_dir2/$first_id.zip $host_and_port/files/$first_id/zip && ok=1 
echo Downloaded $tmp_dir2/$first_id.zip

if [ $ok = "1" ]; then
  echo "OK    GET /files/$first_id/zip" >> $resultfile
else
  set +x
  printf "GET /files/$first_id/zip ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

filename=`zipinfo $tmp_dir2/$first_id.zip | grep File | awk '{ print $9 }'`
ok=0
cd $tmp_dir2 && unzip $tmp_dir2/$first_id.zip && cd - && ok=1

if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$first_id/zip ${RED}TEST FAILED${NC}\n"
  exit 1
fi

diff $tmp_dir2/$filename $tmp_dir/$filename || ok=0
if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$first_id/zip ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi


# ------------------------------------------------------------------------------
# Get mrufiles zip file
# ------------------------------------------------------------------------------
ok=0
rm -f $tmp_dir2/*
curl --output $tmp_dir2/mrufiles.zip $host_and_port/mrufiles/zip && ok=1 

if [ $ok = "0" ]; then
  set +x
  printf "GET /mrufiles/zip ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

ok=0
cd $tmp_dir2 && unzip ./mrufiles.zip && rm mrufiles.zip && cd - && ok=1

if [ $ok = "0" ]; then
  set +x
  printf "GET /mrufiles/zip ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

ok=0
cd $tmp_dir2
ls `curl $host_and_port/mrufiles | grep File | sed "s/\"//g" | sed "s/,//g" | awk '{print $2}'` && ok=1
filescount=`ls *.txt | wc -w`
cd -

if [ $ok = "1" ] && [ $filescount = "3" ]; then
  echo "OK    GET /mrufiles/zip" >> $resultfile
else
  set +x
  printf "GET /mrufiles/zip ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

# ------------------------------------------------------------------------------
# JSON validations 
# ------------------------------------------------------------------------------

#GET /files
ok=0
curl $host_and_port/files > ${tmp_dir}/files.json && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

ok=0
eval "$jsonvalidator $tmp_dir/files.json && ok=1" >> $resultfile
if [ $ok = "0" ]; then
  set +x
  printf "GET /files ${RED}JSON VALIDATION FAILED${NC}\n" >&2
  exit 1
fi


#GET /mrufiles
ok=0
curl $host_and_port/mrufiles > ${tmp_dir}/mrufiles.json && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files ${RED}TEST FAILED${NC}\n" >&2
  exit 1
fi

ok=0
eval "$jsonvalidator $tmp_dir/mrufiles.json && ok=1" >> $resultfile
if [ $ok = "0" ]; then
  set +x
  printf "GET /files ${RED}JSON VALIDATION FAILED${NC}\n" >&2
  exit 1
fi

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

curl 127.0.0.1:8080/mrufiles -v
ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null || ok=1
 if [ $ok = "0" ]; then
   set +x
   printf "GET /files/$first_id ${RED}%first_id should not be in the mrulist now${NC}\n" >&2
   exit 1
fi

# make sure timestamp distance at least is 1 sec
sleep 1.5

ok=0
curl --output $tmp_dir2/$id.zip $host_and_port/files/$id/zip && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$id ${RED}VALIDATION FAILED${NC}\n" >&2
  exit 1
fi

ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$id ${RED}TIMESTAMP VALIDATION FAILED${NC}\n" >&2
  exit 1
fi

echo "TS OK  GET /files/$id/zip" >> $resultfile

#GET /file/<id>
#check that the first_id related entry now is into mrufiles list

id=`echo -n File05.txt | sha256sum  | awk '{print $1}'`

ok=1
curl $host_and_port/mrufiles | grep $id && ok=0
 if [ $ok = "0" ]; then
   set +x
   printf "GET /files/$id ${RED} should not be in the mrulist now${NC}\n" >&2
   exit 1
fi

# make sure timestamp distance at least is 1 sec
sleep 1.5

ok=0
curl $host_and_port/files/$id && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$second_id ${RED}VALIDATION FAILED${NC}\n" >&2
  exit 1
fi

ok=0
curl $host_and_port/mrufiles | grep $id >/dev/null && ok=1
if [ $ok = "0" ]; then
  set +x
  printf "GET /files/$second_id ${RED}TIMESTAMP VALIDATION FAILED${NC}\n" >&2
  exit 1
fi


echo "TS OK  GET /files/$second_id" >> $resultfile

set +x

# ------------------------------------------------------------------------------
# Show results
# ------------------------------------------------------------------------------
clear
echo ------------------------------------
printf "${GREEN}TEST SUCCEDED${NC}\n"
echo ------------------------------------
cat $resultfile


# ------------------------------------------------------------------------------
# Cleanup temporary files
# ------------------------------------------------------------------------------
rm -rf $tmp_dir
rm -rf $tmp_dir2
