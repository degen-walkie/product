# The platform-independent firmware core (UI, venue clients, signing) plus the
# vendored crypto and JSON libraries. Shared with the WebAssembly simulator build.
set(FW ${CMAKE_CURRENT_LIST_DIR}/../..)
file(GLOB_RECURSE CORE_SRCS ${FW}/core/*.c)
set(TP ${FW}/third_party)
idf_component_register(
    SRCS ${CORE_SRCS}
         ${TP}/cjson/cJSON.c
         ${TP}/monocypher/monocypher.c ${TP}/monocypher/monocypher-ed25519.c
         ${TP}/secp256k1/src/secp256k1.c ${TP}/secp256k1/src/precomputed_ecmult.c ${TP}/secp256k1/src/precomputed_ecmult_gen.c
    INCLUDE_DIRS ${FW}/core ${FW}/core/include ${FW}/core/crypto ${FW}/core/venues ${FW}/core/ui ${FW}/core/fonts
                 ${TP}/cjson ${TP}/monocypher ${TP}/secp256k1/include
    REQUIRES lvgl__lvgl)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ENABLE_MODULE_RECOVERY=1 ECMULT_WINDOW_SIZE=4 COMB_BLOCKS=11 COMB_TEETH=6)
set_source_files_properties(${TP}/secp256k1/src/secp256k1.c ${TP}/secp256k1/src/precomputed_ecmult.c
    ${TP}/secp256k1/src/precomputed_ecmult_gen.c PROPERTIES COMPILE_FLAGS "-w")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation -Wno-unused-function)
