#!/bin/bash 

echo -----------------------
echo httpsrv functional test
echo -----------------------

# ------------------------------------------------------------------------------
# Create a temporary dir and initializes some file vars
# ------------------------------------------------------------------------------

tmp_dir=$(mktemp -d -t resources-XXXXXXXXXX)

if [ -d "$tmp_dir" ]; then
  echo "Created ${tmp_dir}..."
else
  echo "Error: ${tmp_dir} not found. Can not continue."
  exit 1
fi

listfile=$tmp_dir"/list.tmp"
resultfile=$tmp_dir"/result.tmp"
allidsfile=$tmp_dir"/allids.tmp"
mruidsfile=$tmp_dir"/mruids.tmp"
threeidsfile=$tmp_dir"/threeids.tmp"
sortedmruidsfile=$tmp_dir"/sortedmruids.tmp"

echo "Creating 10 files in $tmp_dir ..."
for i in {01..10}
do
    echo hello > "$tmp_dir/File${i}.txt"
done

rm -f $resultfile

# ------------------------------------------------------------------------------
# Uploading all the files via http
# ------------------------------------------------------------------------------

echo "Upload files into store via the http server"
listcontent=`find $tmp_dir -type f -name *.txt -exec curl -F name=@{} localhost:8080/store \;`

echo $listcontent | sed -r "s/\}/\\n/g" > $listfile

chk=`cat $listfile | grep id | grep name | grep timestamp | wc -l`

if [ $chk = "10" ]; then
  echo "POST /store TEST PASSED" >> $resultfile
else
  echo "POST /store TEST FAILED"
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
  curl localhost:8080/files/`echo "$p" | awk '{print $3}';` | grep id | awk '{print $2}' >> $allidsfile && ok=1

  if [ $ok = "1" ]; then
    id=`echo $p | awk '{print $3 " for " $5}';`
    echo "GET  /files/$id TEST PASSED" >> $resultfile
  else
    echo "GET  /files/<id> TEST FAILED"
    exit 1
  fi
done < $listfile

sed -i "s/\"//g" $allidsfile
sed -i "s/,//g" $allidsfile
sed -i '/^$/d' $allidsfile

# ------------------------------------------------------------------------------
# Select 3 mru from local list
# ------------------------------------------------------------------------------

tail -n 3 $allidsfile | sort > $threeidsfile

echo "Getting the mrufils from http server" 
rm -f $mruidsfile


# ------------------------------------------------------------------------------
# Query the server for getting the mrufiles
# ------------------------------------------------------------------------------

curl localhost:8080/mrufiles | grep id >> $mruidsfile
sed -i "s/\"//g" $mruidsfile
sed -i "s/,//g" $mruidsfile
sed -i '/^$/d' $mruidsfile
 
rm -f sorted_$mruidsfile

cp $mruidsfile $sortedmruidsfile
cat $sortedmruidsfile | awk '{print $2}' > $mruidsfile
cat $mruidsfile | sort > $sortedmruidsfile

ok=0
diff $threeidsfile $sortedmruidsfile && ok=1

if [ $ok = "1" ]; then
  echo "GET  /mrufiles TEST PASSED" >> $resultfile
else
  echo "GET  /mrufiles TEST FAILED"
  exit 1
fi

# ------------------------------------------------------------------------------
# Show results
# ------------------------------------------------------------------------------

echo ------------------------------------
echo "TEST SUCCEDED"
echo ------------------------------------
cat $resultfile

rm -rf $tmp_dir
