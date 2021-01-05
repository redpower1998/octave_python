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


>test1=pythonImport("pythondemo1")
>test1.add(1,2)


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



# Octave_python

#### 介绍
为Octave增加了Python接口，可以在Octave中直接调用Python的资源.
Octave原有Python函数，但是这个方式限制颇多。作为对比Octave有一个功能好得多的Java接口，可以直接使用Java的类。并可以与Octave良好集成。
当前项目是在Octave中直接增加了Python接口，这个接口类似于Java接口，使用方式均类似。要不原有的Python函数更友好。 

例如可以实现如下计算：

> matplot=pythonImport("matplotlib.pyplot")  
> x1=0:0.1:10  
> x2=sin(x1)  
> matplot.plot(x1,x2)  
> matplot.show()  
  
其中pythonImport 类似于Python的import.
在此之后，就可以用Octave语法去调用Python的资源了， 在上例中，导入了 Python的 matplotlib 库，使用Octave构建了数据，用matplotlib显示图表

#### 本项目并没有完成， 只实现了主要的功能。 因此请不要用在生产环境。

如果遇到问题可以告诉我，也可以自己尝试修改。欢迎任何人修改并提交代码。

原版（没有添加Python接口的版本）Octave源代码可以从地址 https://hg.savannah.gnu.org/hgweb/octave/archive/tip.zip下载

#### 软件架构
模仿Java接口实现Python接口


#### 编译教程

与编译Octave一样，当前版本基于比较新的Octave版本。版本号显示为:
GNU Octave, version 7.0.0
Copyright (C) 2020 The Octave Project Developers.


1.  运行 ./bootstrap
2.  运行 ./configure
3.  运行 make
4.  如果安装，运行make install

#### 使用说明

1.  安装后可以直接运行图形界面
2.  也可以编译完成后，运行./run-octave

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


