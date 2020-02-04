HEADER=lib/evaluators.h
rm $HEADER
echo "#ifndef __ALL_HEADERS__" > $HEADER
echo "#define __ALL_HEADERS__" >> $HEADER
for file in evaluation/lib/*.h
do
    echo "#include \"$file\"" >> $HEADER
done
echo "#endif" >> $HEADER
