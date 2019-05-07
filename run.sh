mkdir /tmp/lfs-mountpoint
chmod 755 /tmp/lfs-mountpoint/
rm disk
./createdisk.sh
./lfs -f /tmp/lfs-mountpoint/
