
destination=$1
path="$destination:~/analyse-instructions"

ssh $destination mkdir -p "analyse-instructions/instructions/runnable"
ssh $destination mkdir -p "analyse-instructions/instructions/faulted"
rsync -r --progress undervolter $path
rsync -r --progress instructions/runnable $path/instructions/.
rsync -r --progress instructions/faulted $path/instructions/.
rsync -r --progress framework $path
rsync -r --progress *.py $path
rsync -r --progress requirements.txt $path

# sudo apt install python3-pip
# sudo apt install linux-utils-generic
# sudo apt install msr-tools