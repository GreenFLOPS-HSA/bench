#!/bin/bash

#echo "------------------------------------------------------------------"
#echo "./copy to_device_type number_of_loops integer_vector_size_MB"
#echo "------------------------------------------------------------------"

echo ""
echo "---------------------------"
echo "4 Bytes Integer Vector copy"
echo "---------------------------"
echo ""

if [ "HSA" = "$1" ]; then
	echo "-----------"
	echo "HSA device"
	echo "-----------"

	echo "From 1 single integer copy to 2GB integers vector"
	echo ""
	for n in 1 10 100 1kB 10kB 100kB 1MB 10MB 50MB 100MB; do
		cmd="./copyHSA $n"
		echo $cmd
		$cmd
	done
	echo ""
	echo "PreFill Cache"
	for n in 1 10 100 1kB 10kB 100kB 1MB 10MB 50MB 100MB; do
		cmd="./copyHSA -c $n"
		echo $cmd
		$cmd
        done
	echo ""

else if [ "OpenCL" = "$1" ]; then
	for dev in CPU GPU; do
		echo "-----------"
		echo "$dev device"
		echo "-----------"
		for n in 1 10 100 1kB 10kB 100kB 1MB 10MB 50MB 100MB; do
			cmd="./copyOpenCL $dev $n"
			echo $cmd
			$cmd
		done
		echo ""
		echo "PreFill Cache"
		for n in 1 10 100 1kB 10kB 100kB 1MB 10MB 50MB 100MB; do
			cmd="./copyOpenCL -c $dev $n"
			echo $cmd
			$cmd
		done
		echo ""
		echo ""
	done
fi
fi

