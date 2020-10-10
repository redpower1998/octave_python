# Octave_python

#### Description
Allow users to use Python objects in Octave in order to take advantage of the rich resources of Python.

A Python interface is added to Octave, and Python resources can be used directly in Octave. Octave has original Python functions, but this method has many limitations. As a comparison Octave has a much better Java interface, you can use Java classes directly. And can be well integrated with Octave. The current project directly adds a Python interface to Octave, which is similar to the Java interface and uses similar methods. More friendly than the original Python functions.

For example:  

> matplot=pythonImport("matplotlib.pyplot")  
> x1=0:0.1:10  
> x2=sin(x1)  
> matplot.plot(x1,x2)  
> matplot.show()  

#### This project has not been completed, only the main functions have been implemented. So please do not use it in a production environment.

If you encounter a problem, you can tell me, or you can try to fix it by yourself. welcome to modify and submit the code.

octave source code download from https://hg.savannah.gnu.org/hgweb/octave/archive/tip.zip

#### Software Architecture
Imitate the Java interface to implement the Python interface

#### Compile

Like compiling Octave, the current version is based on the newer Octave version. 

The version number is displayed as:
GNU Octave, version 7.0.0
Copyright (C) 2020 The Octave Project Develo

> 1.  ./bootstrap 
> 2.  ./configure 
> 3.  make 
> 4.  make install 

#### Instructions

1. You can run the graphical interface directly after installation
2. You can also run ./run-octave after compiling

#### Contribution

1.  Fork the repository
2.  Create Feat_xxx branch
3.  Commit your code
4.  Create Pull Request

