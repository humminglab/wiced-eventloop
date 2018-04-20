# Wiced Eventloop

Implement peripheral devices and network protocol with eventloop


## How to Build 

* Download [WICED Studio](http://www.cypress.com/products/wiced-software) and install it
* Goto wiced project directory and copy this project in app directory 
```sh
cd $WICED_PROJECT/app
git clone https://github.com/humminglab/wiced-eventloop.git
```
* Build example code 
```sh
cd $WICED_PROJECT
./make eventloop.example-${PLATFORM}-NetX

