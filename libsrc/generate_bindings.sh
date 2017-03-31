#!/bin/sh

echo "Removing Old Things"
rm -f *.py
rm -f *.pm
rm -f *.java
rm -f *.cs

echo "Python"
swig -python rlib.i
rm -f ../bindings/python/python.c ../bindings/interfaces/rlib.py
mv rlib_wrap.c ../bindings/python/python.c
mv rlib.py ../bindings/interfaces/rlib.py

echo "Perl"
swig -perl rlib.i
rm -f ../bindings/perl/perl.c ../bindings/interfaces/rlib.pm
mv rlib_wrap.c ../bindings/perl/perl.c
mv rlib.pm ../bindings/interfaces/rlib.pm

echo "Java"
swig -java rlib.i
rm -f ../bindings/java/java.c ../bindings/interfaces/*.java
mv rlib_wrap.c ../bindings/java/java.c
mv *.java ../bindings/interfaces/

echo "C Sharp"
swig -csharp rlib.i
rm -f ../bindings/csharp/csharp.c ../bindings/interfaces/*.cs
mv rlib_wrap.c ../bindings/csharp/csharp.c
sed -e '/DllImport/s/\"rlib\"/\"rlibcsharp\"/g' rlibPINVOKE.cs > foo
rm -f rlibPINVOKE.cs
mv foo rlibPINVOKE.cs
mv *.cs ../bindings/interfaces/

echo "Removing Newly Generated Things"
rm -f *.py
rm -f *.pm
rm -f *.java
rm -f *.cs
