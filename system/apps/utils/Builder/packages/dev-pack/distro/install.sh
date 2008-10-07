#! /bin/sh

packages='flex bison binutils gcc nasm make m4 libtool gdb patch cvs Builder pkg-config doxygen cscope splint'

if [ "$USER" != "root" ]
then
	echo "The installation is not being run by the administrative user."
	echo "Installation aborted."
	echo "You can log in as the \"root\" user and try again."
	exit 1
fi

echo "This will install the packages contained in the developer pack."
echo "Previously installed packages of the same name will be removed first."
echo ""
read -p "Do you want to continue (y/N)? " -e answer

if [ "$answer" != "y" ]
then
	echo "Installation aborted."
	exit 1
fi

echo ""

for package in $packages
do
	if [ -e "/usr/$package" ]
	then
		echo "Uninstalling existing /usr/$package"
		package unregister /usr/$package
		rm -r /usr/$package
		sync
	fi
done

echo ""

for package in `ls *.resource`
do
	echo "Installing $package"
	unzip $package -d /usr
	sync
done

for package in $packages
do
	echo ""
	echo "Registering $package"
	package register /usr/$package
	sync
done

echo ""
echo "Done."
#echo "To fully initialise BinUtils and GCC for compiling software, please reboot"
#echo "the computer."
