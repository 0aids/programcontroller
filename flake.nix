{
  description = "C++ wlroots development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = import nixpkgs {inherit system;};

        # List of dependencies for a typical wlroots compositor
        wlroots-dependencies = with pkgs; [
          # Core Wayland and rendering libraries
          wlroots
          wayland
          wayland-protocols
          wayland-scanner
          mesa # Provides libGL
          libdrm
          cairo
          pango
          pixman
          clang

          # Input and keyboard handling
          libinput
          libxkbcommon

          # Xwayland support for running X11 applications
          xwayland
          xorg.libxcb
          xorg.xcbutilwm
          bear
          libei
        ];

        # List of build tools
        build-tools = with pkgs; [
          gnumake
          gdb # The GNU Debugger
          pkg-config
          socat
        ];
      in {
        # `nix build`
        packages.default = pkgs.stdenv.mkDerivation {
          name = "testcomp";
          src = pkgs.lib.cleanSource ./.;
          # Tools needed at build time
          nativeBuildInputs = build-tools ++ [pkgs.wayland-scanner]; # Add wayland-scanner here

          # Libraries to link against
          buildInputs = wlroots-dependencies;

          # Custom build phase to generate the protocol header
          buildPhase = ''
            # Generate the C header for the xdg-shell protocol
            mkdir -p protocol_headers
            ${pkgs.wayland-scanner}/bin/wayland-scanner client-header \
              ${pkgs.wayland-protocols}/share/wayland-protocols/unstable/xdg-shell/xdg-shell.xml \
              protocol_headers/xdg-shell-protocol.h

            # Now run the make command
            make CFLAGS="-I$(pwd)/protocol_headers"
          '';
        };

        # `nix develop`
        devShells.default = pkgs.mkShell {
          name = "cpp-wlroots-dev-shell";

          # All build tools and dependencies are available in the shell
          packages =
            build-tools
            ++ wlroots-dependencies
            ++ [
              # Add extra development tools here
              pkgs.valgrind # For memory debugging
            ];

          shellHook = ''
            echo "Entered C++ wlroots development shell."
            echo "Compiler and dependencies are now in your PATH."
          '';
        };

        # `nix fmt`
        formatter = pkgs.nixfmt-rfc-style;
      }
    );
}
