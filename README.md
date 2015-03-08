# Air

The glue between the modbus and airweb

# For Maintenance

## Getted Started

1.  Configure server ip, client ip and port

        $ mv config_example.h config.h

    Please edit SERVER_PORT, CLIENT_IP, CLIENT_PORT in config.h

2.  Compile

    The [nitrogen compiler](https://github.com/xsoameix/nitrogen)
    is used to generate c code, please install it first.

        $ cmake .
        $ make

3.  Then you can simply run server:

        $ ./air
        Listening on port 2015

## Bitbucket SSL key

    $ ssh-keygen -t rsa -C "air@airweb.csie.ncu.edu.tw"
    Generating public/private rsa key pair.
    Enter file in which to save the key (/home/xxx/.ssh/id_rsa): /home/xxx/air.key/id_rsa
    Enter passphrase (empty for no passphrase): [Enter]
    Enter same passphrase again: [Enter]

Please upload `id_rsa.pub` to bitbucket and deploy it to air

    $ scp id_rsa id_rsa.pub air:/tmp
    $ setfacl -m u:docker:r /tmp/{id_rsa,id_rsa.pub}
    $ cp -t air/.ssh /tmp/{id_rsa,id_rsa.pub}
    $ chmod 600 air/.ssh/id_rsa
    $ rm /tmp/{id_rsa,id_rsa.pub}


## Security Setting

![Security Setting](raw/master/deploy/security_setting.png)

The password is saved in the file `deploy/pass`.
If you update the password, please remember to update the file `deploy/pass`.
PS. Although I set the password, the web page is still fully OPEN orz...

# For Development

## Generate fixtures

    $ ./script/save_fixtures

The file `fixtures` is saved.

## Test

    $ ./test/test_air
