# BiORAM-SGX
A Practical Privacy-Preserving Data Analysis for Personal Genome by Intel SGX.


# Abstract
Intel SGX is a technology that can executes programs securely using Enclave, secure region on DRAM created by Intel's CPU. But, it is difficult to implement programs using Intel SGX. BiORAM-SGX enable to implement statistical analysis for personal genome data easily and flexibly using Intel SGX. 

In this system, when client request to analyze personal genome data, they get only result. During analysis, data do not leak to client and server, and the analysis procedures do not leak to the server. BiORAM-SGX deploys JavaScript interpreter on Enclave to analyze data flexibly and protect personal genome data. 
Interpreter has functions of statisical analysis for bioinformatics. Therefore, it is easy for client to imprement various kind of statistical programs. BiORAM-SGX stores personal genome data with encryption, and decrypt it only on Enclave. BiORAM-SGX uses [Path ORAM](https://dl.acm.org/doi/abs/10.1145/2508859.2516660) to get encrypted personal genome data quickly and securely.

+ Client: people who analyze personal genome data.
+ Data Owner: people who provide SGX Server with personal genome data.
+ SGX Server: server that has environment using Intel SGX. We assume that SGX Server is malicious.

<img src="https://github.com/cBioLab/BiORAM-SGX/blob/master/figure/overview.png" width=800/>



# Demo
[![BiORAM-SGX](https://github.com/cBioLab/BiORAM-SGX/blob/master/figure/BiORAM-SGX_demo_top.png)](http://www.youtube.com/watch?v=SUXBFdLGHHA "BiORAM-SGX")
※ This demo movie is older than latest version of BiORAM-SGX. Therefore, some of implementation on this movie are a little different from latest specification.



# Installation Requirements
+ BiORAM-SGX needs "linux-sgx" and "linux-sgx-driver". Install them from following site.
    + [linux-sgx](https://github.com/intel/linux-sgx): ver. 2.5
    + [linux-sgx-driver](https://github.com/intel/linux-sgx-driver)


+ BiORAM-SGX also needs following libraries.
```
apt install sqlite3
apt install libsqlite3-dev
apt-get install libcurl4-openssl-dev
```


+ Run the following command to get your system's OpenSSL version. It must be at least 1.1.0:
```
openssl version
```

+ If necessary, download the source for the latest release of OpenSSL 1.1.0, then build and install it into a _non-system directory_ such as /opt (note that both `--prefix` and `--openssldir` should be set when building OpenSSL 1.1.0). For example:
```
wget https://www.openssl.org/source/openssl-1.1.0i.tar.gz
tar xf openssl-1.1.0i.tar.gz
cd openssl-1.1.0i
./config --prefix=/opt/openssl/1.1.0i --openssldir=/opt/openssl/1.1.0i
make
sudo make install
```



# Installation
```
cd ~
git clone git@github.com:cBioLab/BiORAM-SGX.git
cd BiORAM-SGX
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/BiORAM-SGX/sample_libcrypto
./bootstrap
./configure --with-openssldir=/opt/openssl/1.1.0i
make
mkdir SGXserver_data
cd SGXserver_data
mkdir upload_data
mkdir ORAM_table
```

+ You should get your service provider id(SPID) and Attestation Report Root CA Certificate(Intel_SGX_Attestation_RootCA.pem).
  + If you get SPID, write it on setting. Check [HERE](https://github.com/intel/sgx-ra-sample#building-the-sample) for detail.
  + Intel_SGX_Attestation_RootCA.pem can get following way.
  ```
  cd ~/BiORAM-SGX/
  wget https://certificates.trustedservices.intel.com/Intel_SGX_Attestation_RootCA.pem
  ```
  
+ If you have any problem, you should check [sgx-ra-sample](https://github.com/intel/sgx-ra-sample).



# Sample Running
## Create database for user verification
At first, create table on `~/BiORAM-SGX/`.
```
cd ~/BiORAM-SGX/
sqlite3 testdb
$ SQLite version x.xx.x 20xx-xx-xx xx:xx:xx
$ Enter ".help" for usage hints.
$ sqlite> create table users(id text, pwhash text);
$ sqlite> .exit
```

Then, register your id and pwhash.
```
cd ~/BiORAM-SGX/
python3 CreateID_pass.py
$ Input userID:   DataOwner
$ Input password: DataOwner
$ Are you sure to register this userID and password[y/n]?: y
python3 CreateID_pass.py
$ Input userID:   Client
$ Input password: Client
$ Are you sure to register this userID and password[y/n]?: y
```

## [Data Owner] Download genome data (1000 genome project)
```
cd ~/BiORAM-SGX/dataowner_data/
wget ftp://ftp-trace.ncbi.nih.gov/1000genomes/ftp/release/20130502/ALL.chr22.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz
gunzip ALL.chr22.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz
```

## [Data Owner] Split and Encrypt genome data
```
cd ~/BiORAM-SGX/dataowner_data/
# Split genome data by nation. Use "xlrd" library.
python SplitVCFData_nation.py 22
# Split nation genome data by each size(102000[byte]: about 100000 byte + padding).
python3 SplitVCFData_size.py ~/BiORAM-SGX/dataowner_data/ ~/BiORAM-SGX/dataowner_data/chr22_GWD/ 22 GWD 100000 2000
python3 SplitVCFData_size.py ~/BiORAM-SGX/dataowner_data/ ~/BiORAM-SGX/dataowner_data/chr22_JPT/ 22 JPT 100000 2000
# Encrypt splitted nation genome data. We use Intel SGX for encryption, but it is not necessary for Data Owner to use Intel SGX in case Data Onwer encrypt them using AES-GCM.
cd EncryptAES_SGX
make
# GWD: Gambian in Western Division, The Gambia
# JPT: Japanese in Tokyo, Japan
./app ~/BiORAM-SGX/dataowner_data/chr22_GWD/ 22 GWD 102000
./app ~/BiORAM-SGX/dataowner_data/chr22_JPT/ 22 JPT 102000
cd ../
cp -r chr22_GWD chr22_JPT ../SGXserver_data/upload_data/
rm ../SGXserver_data/upload_data/chr22_GWD/AES_SK.key
rm ../SGXserver_data/upload_data/chr22_JPT/AES_SK.key
```


## ※ Shortcut for download, split and encrypt genome data.
Above commands take about 10 minutes because genome data of chromosome 22 is huge. If you use following commands, reduce time.
```
cd ~/BiORAM-SGX/dataowner_data/
# short size of genome data.
gunzip *.gz
python3 SplitVCFData_size.py ~/BiORAM-SGX/dataowner_data/ ~/BiORAM-SGX/dataowner_data/chr22_GWD/ 22 GWD 100000 2000
python3 SplitVCFData_size.py ~/BiORAM-SGX/dataowner_data/ ~/BiORAM-SGX/dataowner_data/chr22_JPT/ 22 JPT 100000 2000
cd EncryptAES_SGX
make
./app ~/BiORAM-SGX/dataowner_data/chr22_GWD/ 22 GWD 102000
./app ~/BiORAM-SGX/dataowner_data/chr22_JPT/ 22 JPT 102000
cd ../
cp -r chr22_GWD chr22_JPT ../SGXserver_data/upload_data/
rm ../SGXserver_data/upload_data/chr22_GWD/AES_SK.key
rm ../SGXserver_data/upload_data/chr22_JPT/AES_SK.key
```


## [Data Owner] Create ORAM structure
+ SGX Server side
```
./run-SGXserver
```

+ Data Owner side
```
./run-client
$ Input your user ID: DataOwner
$ Input your ID's password: DataOwner
$ (If you do not have key, push ENTER only.)
$ Input your SK filename: ./dataowner_data/chr22_GWD/AES_SK.key
$ Input your JavaScript code: ./dataowner_data/ORAMinit_GWD.js
---
./run-client
$ Input your user ID: DataOwner
$ Input your ID's password: DataOwner
$ (If you do not have key, push ENTER only.)
$ Input your SK filename: ./dataowner_data/chr22_JPT/AES_SK.key
$ Input your JavaScript code: ./dataowner_data/ORAMinit_JPT.js
```

## [Client] Analyze genome data
+ SGX Server side
```
./run-SGXserver
```

+ Client side
```
./run-client
$ Input your user ID: Client
$ Input your ID's password: Client
$ (If you do not have key, push ENTER only.)
$ Input your SK filename: [ENTER]
$ Input your JavaScript code: ./client_data/fisher.js
```

Client sample .js codes are as follows.
+ fisher.js: sample code to execute fisher's exact test.
+ LR.js:     sample code to execute logistic regression(100 positions).
+ PCA.js:    sample code to execute PCA(100 positions -> 2 dimension).
+ LR_PCA.js: execute LR(10 positions) -> select 5 positions that have high relation between GWD and JPT -> PCA(5 positions -> 2 dimension) -> save result as file.  
It can visualize as follows. Because sample positions are quite a few, classification is not proper.(If you check proper classification, see [demo](https://github.com/cBioLab/BiORAM-SGX#demo).) 
  ```
  cd ~/BiORAM-SGX/client_data/
  python Visualize_data.py
  ```



# Benchmark(2020/02/20)
## Machine Spec
+ OS: Ubuntu 18.04.3 LTS
+ CPU: Intel Core i7-7700K CPU @ 4.20GHz
+ memory: 16GB
+ [Intel SGX for Linux*](https://github.com/intel/linux-sgx/) 2.5, [Intel SGX Linux* Driver](https://github.com/intel/linux-sgx-driver/) 2.5

## Parameters
+ Z(see detail on [Path ORAM](https://dl.acm.org/doi/abs/10.1145/2508859.2516660) paper.): 6
+ StackMaxSize: 4[MB] (4,000,000 byte)
+ HeapMaxSize: 96[MB] (96,000,000 byte)
+ Data: 1000 Genome Project data, espwcially 2 nations.
  + GWD: Gambian in Western Division, The Gambia
  + JPT: Japanese in Tokyo, Japan

Genome data size are as follows.

|                      | AllGenome(JPT) | AllGenome(GWD) | chr1(JPT) | chr1(GWD) | chr22(JPT) | chr22(GWD) |
|:--------------------:|:--------------:|:--------------:|:---------:|:---------:|:----------:|:----------:|
|    Data size [GB]    |      35.8      |      38.6      |    2.76   |    2.97   |    0.471   |    0.508   |
| num of splitted data |     384758     |     415536     |   29658   |   32006   |    5062    |    5463    |


## Case1: AllGenome
We create ORAM Trees using all human chromosome, each nation(JPT, GWD).
+ Fisher

|   process   | time [sec] |
|:-----------:|:----------:|
| File Search |  4.372849  |
|   Analyze   |  0.0273248 |
|    Total    |  4.401838  |

+ LR  
Using gradient descent, regularization.

<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">47.97443</td>
    <td class="tg-c3ow">216.4722</td>
    <td class="tg-c3ow">406.3569</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.0052505</td>
    <td class="tg-c3ow">0.022678</td>
    <td class="tg-c3ow">0.04415015</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">47.98099</td>
    <td class="tg-c3ow">216.4971</td>
    <td class="tg-c3ow">406.40365</td>
  </tr>
</table>

+ PCA  
In PCA, we use only JPT data, using power method.
<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">19.74556</td>
    <td class="tg-c3ow">101.20553</td>
    <td class="tg-c3ow">237.0048</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.0002727</td>
    <td class="tg-c3ow">0.0028131</td>
    <td class="tg-c3ow">0.0117333</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">19.74735</td>
    <td class="tg-c3ow">101.21001</td>
    <td class="tg-c3ow">237.0183</td>
  </tr>
</table>

## Case2: chromosome 1
We create ORAM Trees using chromosome 1, each nation(JPT, GWD).
+ Fisher

|   process   | time [sec] |
|:-----------:|:----------:|
| File Search |  1.4665754 |
|   Analyze   |  0.0001375 |
|    Total    |  1.4682056 |

+ LR  
Using gradient descent, regularization.
<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">5.742125</td>
    <td class="tg-c3ow">28.30003</td>
    <td class="tg-c3ow">64.83146</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.0055113</td>
    <td class="tg-c3ow">0.022171</td>
    <td class="tg-c3ow">0.0434385</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">5.748933</td>
    <td class="tg-c3ow">28.32372</td>
    <td class="tg-c3ow">64.87664</td>
  </tr>
</table>

+ PCA  
In PCA, we use only JPT data, using power method.
<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">2.47331</td>
    <td class="tg-c3ow">13.19456</td>
    <td class="tg-c3ow">27.24546</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.006414</td>
    <td class="tg-c3ow">0.0059026</td>
    <td class="tg-c3ow">0.0153582</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">2.475577</td>
    <td class="tg-c3ow">13.20257</td>
    <td class="tg-c3ow">27.26291</td>
  </tr>
</table>

## Case3: chromosome 22
We create ORAM Trees using chromosome 22, each nation(JPT, GWD).
+ Fisher

|   process   | time [sec] |
|:-----------:|:----------:|
| File Search |  0.2158026 |
|   Analyze   |  0.0274049 |
|    Total    |  0.244528  |

+ LR  
Using gradient descent, regularization.
<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">3.184544</td>
    <td class="tg-c3ow">22.78428</td>
    <td class="tg-c3ow">39.85593</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.0060702</td>
    <td class="tg-c3ow">0.0235689</td>
    <td class="tg-c3ow">0.0479591</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">3.191978</td>
    <td class="tg-c3ow">22.80935</td>
    <td class="tg-c3ow">39.90606</td>
  </tr>
</table>

+ PCA  
In PCA, we use only JPT data, using power method.
<table class="tg">
  <tr>
    <th class="tg-c3ow" rowspan="2"></th>
    <th class="tg-c3ow" colspan="3">number of positions</th>
  </tr>
  <tr>
    <td class="tg-c3ow">10</td>
    <td class="tg-c3ow">50</td>
    <td class="tg-c3ow">100</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Fille Search [sec]</td>
    <td class="tg-c3ow">1.470165</td>
    <td class="tg-c3ow">9.026763</td>
    <td class="tg-c3ow">15.40194</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Analyze [sec]</td>
    <td class="tg-c3ow">0.0006192</td>
    <td class="tg-c3ow">0.0039763</td>
    <td class="tg-c3ow">0.0133208</td>
  </tr>
  <tr>
    <td class="tg-c3ow">Total [sec]</td>
    <td class="tg-c3ow">1.472607</td>
    <td class="tg-c3ow">9.032648</td>
    <td class="tg-c3ow">15.41728</td>
  </tr>
</table>



# License
BiORAM-SGX is released under the [MIT License](https://opensource.org/licenses/mit-license). See [LICENSE](https://github.com/cBioLab/BiORAM-SGX/blob/master/LICENSE) for details.

Licenses of external libraries are listed as follows.
+ [linux-sgx](https://github.com/intel/linux-sgx/blob/master/License.txt)
+ [linux-sgx-driver](https://github.com/intel/linux-sgx-driver/blob/master/License.txt)
+ [OpenSSL](https://www.openssl.org/source/license.html)
+ [SQLite](https://www.sqlite.org/copyright.html)
+ [tiny-js](https://github.com/gfwilliams/tiny-js/blob/master/LICENSE)



# Acknowledgement
We thank Mr.Ao Sakurai for fruitful discussions.



# Contact
Daiki Iwata(d_iwata@ruri.waseda.jp)
