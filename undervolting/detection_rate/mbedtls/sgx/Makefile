######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk

LIB_DIR=../../../libraries

App_Name := main
App_C_Flags := -fPIC -Wno-attributes -I$(SGX_SDK)/include -DDEBUG -UNDEBUG -UEDEBUG -g -I$(LIB_DIR)/undervolting -mrdrnd
App_Link_Flags := -L$(LIB_DIR)/undervolting -lundervolting

Enclave_C_Flags := -nostdinc -fvisibility=hidden -fpie -fstack-protector -Os -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport -g

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
Enclave_Link_Flags := -g -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_SDK)/lib64	-L../poc/mbedtls/library -L../poc \
	-Wl,--whole-archive -lsgx_trts -lcrypto -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tservice -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=enclave.lds 

	
	 
all: $(App_Name) enclave.signed.so


######## Unrusted ########

enclave_u.c: enclave.edl
	@$(SGX_SDK)/bin/x64/sgx_edger8r --untrusted enclave.edl --search-path . --search-path $(SGX_SDK)/include

enclave_u.o: enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@

main.o: main.cpp
	@$(CC) $(App_C_Flags) -c $< -o $@

$(App_Name): enclave_u.o main.o
	@$(CC) $^ -o $@ -L$(SGX_SDK)/lib64 $(App_Link_Flags) -lsgx_urts -lpthread -lsgx_uae_service 


######## Trusted ########

enclave_t.c: enclave.edl
	@$(SGX_SDK)/bin/x64/sgx_edger8r --trusted enclave.edl --search-path . --search-path $(SGX_SDK)/include

enclave_t.o: enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@

enclave.so: enclave_t.o
	@$(CC) $^ -o $@ $(Enclave_Link_Flags)

key.pem:
	openssl genrsa -out key.pem -3 2048

enclave.signed.so: enclave.so key.pem
	@$(SGX_SDK)/bin/x64/sgx_sign sign -key key.pem -enclave enclave.so -out $@ -config config.xml

.PHONY: clean

clean:
	@rm -f $(App_Name) enclave.so enclave.signed.so *.o *_t.c *_t.h *_u.c *_u.h
