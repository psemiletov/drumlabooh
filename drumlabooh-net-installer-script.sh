#!/bin/bash
set -euo pipefail

# Function to check if a command is available
check_command() {
    local cmd="$1"
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "Error: $cmd is required but not installed." >&2
        exit 1
    fi
}

# Function to fetch the latest release version from GitHub API
get_latest_release() {
    local repo="$1"
    local url="https://api.github.com/repos/$repo/releases/latest"
    local response

    # Check for network connectivity
    ping -c 1 api.github.com >/dev/null 2>&1 || { echo "No internet connection" >&2; return 1; }

    # Fetch the latest release with curl
    response=$(curl -f -s -H "Accept: application/vnd.github+json" "$url")
    if [ $? -ne 0 ]; then
        echo "Failed to fetch release info for $repo" >&2
        return 1
    fi

    # Parse the tag_name using jq
    local version
    version=$(echo "$response" | jq -r '.tag_name')
    if [ -z "$version" ] || [ "$version" == "null" ]; then
        echo "Failed to parse version from $repo release" >&2
        return 1
    fi

    echo "$version"
}

# Function to determine user home directory
userHomeDir() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -n "${XDG_CONFIG_HOME:-}" ]; then
            echo "$XDG_CONFIG_HOME"
            return
        fi
    fi
    echo "${HOME:-}"
}

# Function to download a file with error checking
downloadFile() {
    local filepath="$1"
    local url="$2"
    local filename=$(basename "$filepath")

    echo "Downloading $filename..."

    # Check network connectivity
    ping -c 1 github.com >/dev/null 2>&1 || { echo "No internet connection " >&2; return 1; }

    # Download with timeout and progress
    if ! curl -f -L --progress-bar --connect-timeout 10 "$url" -o "$filepath"; then
        echo "Failed to download $url" >&2
        rm -f "$filepath"
        return 1
    fi

    # Check file size
    if [ ! -s "$filepath" ]; then
        echo "Downloaded file is empty: $filename" >&2
        rm -f "$filepath"
        return 1
    fi

    # Check ZIP integrity
    if [[ "$filename" == *.zip ]]; then
        if ! unzip -tq "$filepath" >/dev/null 2>&1; then
            echo "Downloaded ZIP file is corrupted: $filename" >&2
            rm -f "$filepath"
            return 1
        fi
    fi

    echo "Successfully downloaded $filename"
}

# Function to unzip an archive with error checking
Unzip() {
    local src="$1"
    local dest="$2"
    local ver="$3"
    local filename=$(basename "$src")

    echo "Unpacking $filename to $dest..."

    # Check if file exists and is a valid ZIP
    if [ ! -f "$src" ]; then
        echo "File not found: $src" >&2
        return 1
    fi

    if ! unzip -tq "$src" >/dev/null 2>&1; then
        echo "Invalid or corrupted ZIP file: $src" >&2
        return 1
    fi

    # Create temporary directory for unpacking
    local temp_dir=$(mktemp -d)

    # Unzip the archive
    if ! unzip -q "$src" -d "$temp_dir"; then
        echo "Failed to unzip $src" >&2
        return 1
    fi

    # Rename drum_sklad-ver to drum_sklad
    if [ -d "$temp_dir/drum_sklad-$ver" ]; then
        mv "$temp_dir/drum_sklad-$ver" "$temp_dir/drum_sklad"
    fi

    # Copy contents to destination
    mkdir -p "$dest"
    local total_files=$(find "$temp_dir" -type f | wc -l)
    local current_file=0

    while IFS= read -r -d '' file; do
        current_file=$((current_file + 1))
        relative_path="${file#$temp_dir/}"
        dest_path="$dest/${relative_path}"

        # Create directory if needed
        mkdir -p "$(dirname "$dest_path")"

        # Copy file
        cp "$file" "$dest_path"

        # Update progress
        percent=$((current_file * 100 / total_files))
        bar_length=$((percent / 2))
        printf "\rUnpacking %s: [%-50s] %d%% (%d/%d)" "$filename" "$(printf '#%.0s' $(seq 1 $bar_length))" "$percent" "$current_file" "$total_files"
    done < <(find "$temp_dir" -type f -print0)

    echo
    echo "Successfully unpacked $filename"
}

# Function to check if running as root
isRoot() {
    [ "$(id -u)" -eq 0 ]
}

main() {
    VER_LOCAL="4"

    # Check for required commands
    check_command "curl"
    check_command "jq"
    check_command "unzip"

    if isRoot; then
        echo "Please run as non-root"
        exit 1
    fi

    local arg=""
    local flag_test=false

    if [ $# -eq 1 ]; then
        arg="$1"
        if [ "$arg" = "test" ]; then
            flag_test=true
            echo "TEST MODE"
        fi
    fi

    echo "Drumlabooh Net Installer $VER_LOCAL"

    home_dir=$(userHomeDir)
    echo "Home directory: $home_dir"

    tempdir=$(mktemp -d)
    echo "Using temporary directory: $tempdir"
    trap 'rm -rf "$tempdir"' EXIT

    echo "Fetching version information..."

    # Fetch latest release versions using GitHub API
    labooh_ver=$(get_latest_release "psemiletov/drumlabooh") || { echo "Failed to get drumlabooh version" >&2; exit 1; }
    kits_ver=$(get_latest_release "psemiletov/drum_sklad") || { echo "Failed to get drum_sklad version" >&2; exit 1; }

    echo "Installing/updating Drumlabooh v.$labooh_ver"
    echo "Installing/updating kits v.$kits_ver"

    # Construct download URLs using fetched versions
    lv2_url="https://github.com/psemiletov/drumlabooh/releases/download/$labooh_ver/drumlabooh.lv2.zip"
    vst_url="https://github.com/psemiletov/drumlabooh/releases/download/$labooh_ver/drumlabooh.vst3.zip"
    lv2m_url="https://github.com/psemiletov/drumlabooh/releases/download/$labooh_ver/drumlabooh-multi.lv2.zip"
    vstm_url="https://github.com/psemiletov/drumlabooh/releases/download/$labooh_ver/drumlabooh-multi.vst3.zip"
    drumkits_url="https://github.com/psemiletov/drum_sklad/archive/refs/tags/$kits_ver.zip"

    source_path_to_lv2_zip="$tempdir/drumlabooh.lv2.zip"
    source_path_to_vst_zip="$tempdir/drumlabooh.vst3.zip"
    source_path_to_lv2m_zip="$tempdir/drumlabooh-multi.lv2.zip"
    source_path_to_vstm_zip="$tempdir/drumlabooh-multi.vst3.zip"
    source_path_to_drum_sklad="$tempdir/drum_sklad.zip"

    dest_lv2_path="$home_dir/.lv2"
    dest_vst_path="$home_dir/.vst3"
    dest_drumsklad_path="$home_dir"

    if $flag_test; then
        dest_lv2_path="${dest_lv2_path}TEST"
        dest_vst_path="${dest_vst_path}TEST"
        dest_drumsklad_path="/STEST"
        echo "Using TEST paths:"
        echo "  LV2: $dest_lv2_path"
        echo "  VST: $dest_vst_path"
        echo "  Kits: $dest_drumsklad_path"
    fi

    # Download and unzip LV2
    echo "Processing LV2 plugin..."
    if ! $flag_test; then
        if downloadFile "$source_path_to_lv2_zip" "$lv2_url"; then
            Unzip "$source_path_to_lv2_zip" "$dest_lv2_path" "$kits_ver"
        else
            echo "Failed to process LV2 plugin " >&2
            exit 1
        fi
    fi

    # Download and unzip LV2-multi
    echo "Processing LV2m plugin..."
    if ! $flag_test; then
        if downloadFile "$source_path_to_lv2m_zip" "$lv2m_url"; then
            Unzip "$source_path_to_lv2m_zip" "$dest_lv2_path" "$kits_ver"
        else
            echo "Failed to process LV2m plugin " >&2
            exit 1
        fi
    fi

    # Download and unzip VST3
    echo "Processing VST3 plugin..."
    if ! $flag_test; then
        if downloadFile "$source_path_to_vst_zip" "$vst_url"; then
            Unzip "$source_path_to_vst_zip" "$dest_vst_path" "$kits_ver"
        else
            echo "Failed to process VST3 plugin " >&2
            exit 1
        fi
    fi

    # Download and unzip VST3m
    echo "Processing VST3m plugin..."
    if ! $flag_test; then
        if downloadFile "$source_path_to_vstm_zip" "$vstm_url"; then
            Unzip "$source_path_to_vstm_zip" "$dest_vst_path" "$kits_ver"
        else
            echo "Failed to process VST3 plugin " >&2
            exit 1
        fi
    fi

    # Download and unzip drum kits
    echo "Processing drum kits..."
    if ! $flag_test; then
        if downloadFile "$source_path_to_drum_sklad" "$drumkits_url"; then
            Unzip "$source_path_to_drum_sklad" "$dest_drumsklad_path" "$kits_ver"
        else
            echo "Failed to process drum kits" >&2
            exit 1
        fi
    fi

    echo "Cleaning up..."
    echo "Installation completed successfully"
}

main "$@"