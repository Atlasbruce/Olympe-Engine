#!/bin/bash
# setup-docs.sh - Helper script for documentation maintenance
# Usage: ./scripts/setup-docs.sh [command]

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
WEBSITE_DIR="$PROJECT_ROOT/website"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    echo -e "${GREEN}[Docs]${NC} $1"
}

print_error() {
    echo -e "${RED}[Error]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[Warning]${NC} $1"
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check prerequisites
check_prerequisites() {
    print_msg "Checking prerequisites..."
    
    local missing=0
    
    if ! command_exists node; then
        print_error "Node.js is not installed"
        missing=1
    else
        print_msg "Node.js: $(node --version)"
    fi
    
    if ! command_exists npm; then
        print_error "npm is not installed"
        missing=1
    else
        print_msg "npm: $(npm --version)"
    fi
    
    if ! command_exists doxygen; then
        print_warning "Doxygen is not installed (required for API docs)"
    else
        print_msg "Doxygen: $(doxygen --version)"
    fi
    
    if [ $missing -eq 1 ]; then
        print_error "Missing required dependencies"
        exit 1
    fi
    
    print_msg "Prerequisites OK"
}

# Install dependencies
install_deps() {
    print_msg "Installing dependencies..."
    cd "$WEBSITE_DIR"
    npm ci
    print_msg "Dependencies installed"
}

# Generate API documentation
generate_api() {
    print_msg "Generating API documentation with Doxygen..."
    
    if ! command_exists doxygen; then
        print_error "Doxygen not found. Install with: sudo apt-get install doxygen graphviz"
        exit 1
    fi
    
    cd "$PROJECT_ROOT"
    doxygen Doxyfile
    
    if [ -d "$WEBSITE_DIR/static/api" ]; then
        print_msg "API documentation generated at website/static/api/"
    else
        print_error "API documentation generation failed"
        exit 1
    fi
}

# Build documentation site
build_docs() {
    print_msg "Building documentation site..."
    cd "$WEBSITE_DIR"
    npm run build
    
    if [ -d "$WEBSITE_DIR/build" ]; then
        print_msg "Documentation built at website/build/"
    else
        print_error "Documentation build failed"
        exit 1
    fi
}

# Show help
show_help() {
    cat << EOF
Olympe Engine Documentation Helper

Usage: ./scripts/setup-docs.sh [command]

Commands:
    check       Check prerequisites
    install     Install dependencies
    api         Generate API documentation (Doxygen)
    build       Build documentation site
    full        Full build (API + Site)
    help        Show this help message

Examples:
    ./scripts/setup-docs.sh check
    ./scripts/setup-docs.sh full

EOF
}

# Main script
main() {
    local command="${1:-help}"
    
    case "$command" in
        check)
            check_prerequisites
            ;;
        install)
            check_prerequisites
            install_deps
            ;;
        api)
            generate_api
            ;;
        build)
            check_prerequisites
            build_docs
            ;;
        full)
            check_prerequisites
            install_deps
            generate_api
            build_docs
            print_msg "Full documentation build complete!"
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            print_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

# Run main
main "$@"
