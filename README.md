![Alysides Header](https://github.com/VleppoOfficial/alysides/blob/main/alysides-header.png "Alysides Header")

## Vleppo Alysides
This repository hosts the Vleppo Alysides core blockchain software that is required to host all Komodo-based blockchains used by the [Vleppo Platform](https://vleppo.com/).

All stable releases used in production / notary node environments are hosted in the 'main' branch. 
- [https://github.com/VleppoOfficial/alysides](https://github.com/VleppoOfficial/alysides/tree/main)

To run Vleppo's non-production chains and receive latest updates that will be integrated to the 'main' branch in the next notary node season, use the 'beta' branch instead.
- [https://github.com/VleppoOfficial/alysides](https://github.com/VleppoOfficial/alysides/tree/beta)

Vleppo Alysides is powered by the [Komodo Platform](https://komodoplatform.com/en), and contains code enhancements from the [Tokel Platform](https://github.com/TokelPlatform/tokel).

## List of Alysides Technologies
- All technologies from the main Komodo Platform codebase, such as:
  - Delayed Proof of Work (dPoW) - Additional security layer and Komodo's own consensus algorithm
  - zk-SNARKs - Komodo Platform's privacy technology for shielded transactions (however, it is unused and inaccessible in any of Vleppo's chains)
  - CC - Custom Contracts to realize UTXO-based "smart contract" logic on top of blockchains
- Enhancements inherited from the Tokel Platform codebase, such as:
  - Improvements to the Tokens & Assets CCs
  - Improvements to Komodo's nSPV technology. nSPV is a super-lite, next-gen SPV technology gives users the ability to interact with their tokens in a decentralized & trust-less fashion on any device, without the inconvenience and energy cost of downloading the entire blockchain.
- Agreements CC, a Komodo Custom Contract allowing fully on-chain digital contract creation & management
- Token Tags CC, a Komodo Custom Contract enabling amendable data logs attached to existing Tokens

## Installation
In order to run any of Vleppo's Komodo-based blockchains as a full node, you must build the Alysides daemon and launch the specified blockchain through it.
If you wish to run a production-grade Vleppo blockchain, make sure you are running it from the 'main' branch of this repository in order to avoid syncing issues.

#### Dependencies
```shell
#The following packages are needed:
sudo apt-get install build-essential pkg-config libc6-dev m4 g++-multilib autoconf libtool ncurses-dev unzip git zlib1g-dev wget curl bsdmainutils automake cmake clang ntp ntpdate nano -y
```

#### Linux
```shell
git clone https://github.com/VleppoOfficial/alysides --branch main --single-branch
cd alysides
./zcutil/fetch-params.sh
./zcutil/build.sh -j$(expr $(nproc) - 1)
#This can take some time.
```

#### OSX
Ensure you have [brew](https://brew.sh/) and Command Line Tools installed.

```shell
# Install brew
/bin/bash -c "$(curl -fSSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# Install Xcode, opens a pop-up window to install CLT without installing the entire Xcode package
xcode-select --install
# Update brew and install dependencies
brew update
brew upgrade
brew tap discoteq/discoteq; brew install flock
brew install autoconf autogen automake
brew update && brew install gcc@8
brew install binutils
brew install protobuf
brew install coreutils
brew install wget
# Clone the Alysides repo
git clone https://github.com/VleppoOfficial/alysides --branch main --single-branch
cd alysides
./zcutil/fetch-params.sh
./zcutil/build-mac.sh -j$(expr $(sysctl -n hw.ncpu) - 1)
# This can take some time.
```

#### Windows
The Windows software cannot be directly compiled on a Windows machine. Rather, the software must be compiled on a Linux machine, and then transferred to the Windows machine. You can also use a Virtual Machine-based installation of Debian or Ubuntu Linux, running on a Windows machine, as an alternative solution.
Use a Debian-based cross-compilation setup with MinGW for Windows and run:

```shell
sudo apt-get install build-essential pkg-config libc6-dev m4 g++-multilib autoconf libtool ncurses-dev unzip git zlib1g-dev wget libcurl4-gnutls-dev bsdmainutils automake curl cmake mingw-w64 libsodium-dev libevent-dev
curl https://sh.rustup.rs -sSf | sh
source $HOME/.cargo/env
rustup target add x86_64-pc-windows-gnu

sudo update-alternatives --config x86_64-w64-mingw32-gcc
# (configure to use POSIX variant)
sudo update-alternatives --config x86_64-w64-mingw32-g++
# (configure to use POSIX variant)

git clone https://github.com/VleppoOfficial/alysides --branch main --single-branch
cd alysides
./zcutil/fetch-params.sh
./zcutil/build-win.sh -j$(expr $(nproc) - 1)
#This can take some time.
```

#### Launch Alysides
Change to the Alysides src directory:

```shell
cd ~/alysides/src
```

Launch the Alysides chain command:

```shell
./alysidesd &
```

Running the alysidesd executable will launch the most up-to-date Vleppo blockchain. If you wish to run a specific blockchain that is not the default, refer to Vleppo Blockchain Specifics below.

Now wait for the chain to finish syncing. This might take while depending on your machine and internet connection. You can check check sync progress by using tail -f on the debug.log file in the blockchain's data directory. Double check the number of blocks you've downloaded with an explorer to verify you're up to the latest block.

Alysides uses CryptoConditions that require launching the blockchain with the -pubkey parameter to work correctly. Once you have completed block download, you will need to create a new address or import your current address. After you have done that, you will need to stop the blockchain and launch it with the -pubkey parameter.

You can use the RPC below to create a new address or import a privkey you currently have.

```shell
./alysides-cli getnewaddress
```

```shell
./alysides-cli importprivkey
```

Once you have completed this, use the validateaddress RPC to find your associated pubkey.

```shell
./alysides-cli validateaddress *INSERTYOURADDRESSHERE*
```

Once you have written down your pubkey, stop the blockchain daemon.

```shell
cd ~/alysides/src
./alysides-cli stop
```

Wait a minute or so for the blockchain to stop, then relaunch the blockchain with the command below. Please remove the ** and replace them with the pubkey of the address you imported.

```shell
cd ~/alysides/src
./alysidesd -pubkey=**YOURPUBKEYHERE** &
```

You are now ready to use the Alysides software to its fullest extent.

## Vleppo Resources
- Vleppo Website: [https://vleppo.com](https://vleppo.com)
- Vleppo Discord: [Invitation](https://discord.gg/PdvqNjkWTd)
- Vleppo Telegram: [Invitation](https://t.me/vleppospeaks)
- Email: [support@vleppo.com](mailto:support@vleppo.com)

## Vleppo Blockchain Specifics
The following is a list of Komodo-based Vleppo blockchains that can be run with the Alysides software, alongside their attributes and exact launch parameters. The list will be updated over time as new blockchains are created for use in the Vleppo Platform.

### VLCGLB1
VLCGLB1 is a "beta" chain, being the first publicly available of its type. The VLCGLB1 coin holds no value, just like other VLC coins, as its primary purpose is testing and facilitating Custom Contract functionality for the Vleppo Application.

NOTE: VLCGLB1 must be run using komodod executables compiled from the 'legacy_v0.5' branch.

```shell
-ac_name=VLCGLB1 -ac_supply=498000000 -ac_reward=1000000000 -ac_blocktime=120 -ac_cc=1 -ac_staked=100 -ac_halving=27600 -ac_decay=85000000 -ac_end=331200 -ac_public=1 -addnode=143.110.242.177 -addnode=143.110.254.96 -addnode=139.59.110.85 -addnode=139.59.110.86
#Example launch command (remove the ** and replace them with the pubkey of the address you imported):
./komodod -ac_name=VLCGLB1 -ac_supply=498000000 -ac_reward=1000000000 -ac_blocktime=120 -ac_cc=1 -ac_staked=100 -ac_halving=27600 -ac_decay=85000000 -ac_end=331200 -ac_public=1 -addnode=143.110.242.177 -addnode=143.110.254.96 -addnode=139.59.110.85 -addnode=139.59.110.86 -pubkey=**YOURPUBKEYHERE** & 
```

- Max Supply: Approximately 500 million VLC
- Block Time: 120 seconds
- Starting Block Reward (Era 1): 10 VLC
- Block reward reduction time period: Every 27600 blocks following Era 1
- Reduction amount: 15%
- Mining Algorithm: 100% Proof-of-Stake

### VLCGLB2
VLCGLB2 is a "candidate" chain, running on current versions of Alysides. The VLCGLB2 coin holds no value, just like other VLC coins, as its primary purpose is facilitating Custom Contract functionality for the Vleppo Application and other offerings.

```shell
-ac_name=VLCGLB2 -ac_supply=500000000 -ac_reward=1 -ac_blocktime=120 -ac_cc=1 -ac_ccenable=167,168,169,228,236,245 -ac_staked=100 -ac_end=1 -ac_public=1 -addnode=143.110.242.177 -addnode=143.110.254.96 -addnode=139.59.110.85 -addnode=139.59.110.86
#Example launch command (remove the ** and replace them with the pubkey of the address you imported):
./komodod -ac_name=VLCGLB2 -ac_supply=500000000 -ac_reward=1 -ac_blocktime=120 -ac_cc=1 -ac_ccenable=167,168,169,228,236,245 -ac_staked=100 -ac_end=1 -ac_public=1 -addnode=143.110.242.177 -addnode=143.110.254.96 -addnode=139.59.110.85 -addnode=139.59.110.86 -pubkey=**YOURPUBKEYHERE** & 
```

- Max Supply: Approximately 500 million VLC
- Block Time: 120 seconds
- Starting Block Reward: None
- Mining Algorithm: 100% Proof-of-Stake

## License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
