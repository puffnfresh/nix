{
  stdenv,
  lib,
  gcc,
  gdb,
  wine64Packages,
  writeText,
  writeScript,
}:

let
  program = "\${workspaceFolder}/build/src/nix/nix.exe";
  baseConfig = {
    name = "winedbg (remote)";
    type = "cppdbg";
    request = "launch";
    cwd = "\${workspaceFolder}";
    inherit program;

    MIMode = "gdb";
    miDebuggerPath = "${lib.getBin gdb}/bin/${stdenv.targetPlatform.config}-gdb";
    miDebuggerServerAddress = "localhost:9939";

    setupCommands = [
      {
        description = "Pretty-printing";
        text = "-enable-pretty-printing";
        ignoreFailures = false;
      }
      {
        description = "Pretty-printing for libstdc++";
        text = builtins.replaceStrings [ "\n" ] [ "; " ] ''
          python sys.path.insert(0, '${lib.getLib gcc.cc}/share/gcc-${gcc.cc.version}/python')
          from libstdcxx.v6.printers import register_libstdcxx_printers
          register_libstdcxx_printers(None)
        '';
        ignoreFailures = false;
      }
      {
        description = "Do not stop for SIGABRT (exceptions)";
        text = "handle SIGABRT nostop noprint noignore";
        ignoreFailures = false;
      }
    ];
  };
  launch = {
    version = "0.2.0";
    inputs = [
      {
        id = "args";
        type = "promptString";
        description = "Arguments to nix.exe";
      }
    ];
    configurations = [
      baseConfig
      (
        baseConfig
        // {
          name = "winedbg (launch nix.exe)";

          debugServerPath = writeScript "nix-winedbg.sh" ''
            #!/usr/bin/env bash
            # cppdbg accepts "environment" and "envFile" but these seem
            # to get passed to miDebuggerPath, and not debugServerPath!
            unset NIX_STORE
            exec ${lib.getExe' wine64Packages.minimal "winedbg"} "$@"
          '';
          debugServerArgs = "--gdb --no-start --port 9939 ${program} \${input:args}";

          # HACK: I don't know why "target remote" doesn't work.
          serverStarted = "Could not create tray window";
          filterStderr = true;
        }
      )
    ];
  };
in
writeText "launch.json" (builtins.toString (builtins.toJSON launch))
