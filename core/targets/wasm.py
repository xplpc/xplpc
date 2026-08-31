# archs
has_wasm32 = True

data = []

# wasm32
if has_wasm32:
    data.extend(
        [
            {
                "arch": "wasm32",
                "conan_arch": "wasm",
                "conan_profile": "xplpc_wasm_profile",
            },
        ]
    )
