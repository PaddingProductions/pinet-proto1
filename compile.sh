if [ ! -d './out' ]; then
	echo './out directory not found, creating ./out directory...'
       	mkdir ./out
fi	      

echo 'Compiling all ".cpp" files in "./src" folder with gcc, storing executable in "out/main"...' 
g++ -o out/main ./src/*.cpp
echo 'Done'
