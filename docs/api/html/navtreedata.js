/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "IGNACC Controller", "index.html", [
    [ "IGNACC_Controller", "index.html", "index" ],
    [ "Code tour", "md_docs_2code-tour.html", [
      [ "Before you start (5 min)", "md_docs_2code-tour.html#autotoc_md17", null ],
      [ "Stop 1 — <span class=\"tt\">main</span>: where everything is built (10 min)", "md_docs_2code-tour.html#autotoc_md19", null ],
      [ "Stop 2 — <span class=\"tt\">relay</span>: switching one pin, safely (10 min)", "md_docs_2code-tour.html#autotoc_md21", null ],
      [ "Stop 3 — <span class=\"tt\">uart_link</span>: bytes in, lines out (8 min)", "md_docs_2code-tour.html#autotoc_md23", null ],
      [ "Stop 4 — <span class=\"tt\">app_fsm</span>: the decision maker (10 min)", "md_docs_2code-tour.html#autotoc_md25", null ],
      [ "Stop 5 — <span class=\"tt\">protocol</span>: text to command, result to text (5 min)", "md_docs_2code-tour.html#autotoc_md27", null ],
      [ "Recap: follow <span class=\"tt\">IN1_ON</span> from end to end", "md_docs_2code-tour.html#autotoc_md29", null ],
      [ "After the tour", "md_docs_2code-tour.html#autotoc_md31", null ]
    ] ],
    [ "Configuration and architecture", "md_docs_2configuration-and-architecture.html", [
      [ "1. The big picture", "md_docs_2configuration-and-architecture.html#autotoc_md34", null ],
      [ "2. Where configuration lives", "md_docs_2configuration-and-architecture.html#autotoc_md36", null ],
      [ "3. The relay channel model (what's different here)", "md_docs_2configuration-and-architecture.html#autotoc_md38", [
        [ "3.1 Kconfig: a count plus one pin per channel", "md_docs_2configuration-and-architecture.html#autotoc_md39", null ],
        [ "3.2 main.c: one list, three compile-time checks", "md_docs_2configuration-and-architecture.html#autotoc_md40", null ],
        [ "3.3 Why 14 is the maximum", "md_docs_2configuration-and-architecture.html#autotoc_md41", null ],
        [ "3.4 How the rest of the firmware adapts", "md_docs_2configuration-and-architecture.html#autotoc_md42", null ]
      ] ],
      [ "4. Other settings", "md_docs_2configuration-and-architecture.html#autotoc_md44", null ],
      [ "5. Extending without modifying the core", "md_docs_2configuration-and-architecture.html#autotoc_md46", null ],
      [ "6. Tests that guard all this", "md_docs_2configuration-and-architecture.html#autotoc_md48", null ],
      [ "7. API reference", "md_docs_2configuration-and-architecture.html#autotoc_md50", null ]
    ] ],
    [ "Two GitHub accounts, one computer, one repo", "md_docs_2github-two-accounts.html", [
      [ "The idea in one minute", "md_docs_2github-two-accounts.html#autotoc_md82", null ],
      [ "Step 1: One SSH key per account", "md_docs_2github-two-accounts.html#autotoc_md84", null ],
      [ "Step 2: Lock down the key files", "md_docs_2github-two-accounts.html#autotoc_md86", [
        [ "Windows", "md_docs_2github-two-accounts.html#autotoc_md87", null ],
        [ "Linux", "md_docs_2github-two-accounts.html#autotoc_md88", null ]
      ] ],
      [ "Step 3: <span class=\"tt\">~/.ssh/config</span>, which key for which account", "md_docs_2github-two-accounts.html#autotoc_md90", null ],
      [ "Step 4: Test both accounts", "md_docs_2github-two-accounts.html#autotoc_md92", null ],
      [ "Step 5: <span class=\"tt\">.gitconfig</span>, who the commits say they're from", "md_docs_2github-two-accounts.html#autotoc_md94", [
        [ "Optional: a different identity per folder", "md_docs_2github-two-accounts.html#autotoc_md95", null ]
      ] ],
      [ "Step 6: Connect the repo to both accounts", "md_docs_2github-two-accounts.html#autotoc_md97", [
        [ "Add the remotes", "md_docs_2github-two-accounts.html#autotoc_md98", null ],
        [ "First push", "md_docs_2github-two-accounts.html#autotoc_md99", null ],
        [ "Make one <span class=\"tt\">git push</span> update both", "md_docs_2github-two-accounts.html#autotoc_md100", null ]
      ] ],
      [ "Everyday use", "md_docs_2github-two-accounts.html#autotoc_md102", null ],
      [ "Setting this up on Linux, start to finish", "md_docs_2github-two-accounts.html#autotoc_md104", null ],
      [ "Passphrases", "md_docs_2github-two-accounts.html#autotoc_md106", null ],
      [ "Troubleshooting", "md_docs_2github-two-accounts.html#autotoc_md108", null ],
      [ "Undo", "md_docs_2github-two-accounts.html#autotoc_md110", null ]
    ] ],
    [ "Unit testing", "md_docs_2unit-testing.html", [
      [ "At a glance", "md_docs_2unit-testing.html#autotoc_md112", null ],
      [ "1. What is tested, and what isn't", "md_docs_2unit-testing.html#autotoc_md114", null ],
      [ "2. Install the tools (Windows 11)", "md_docs_2unit-testing.html#autotoc_md116", [
        [ "2.1 C compiler: MinGW-w64 GCC", "md_docs_2unit-testing.html#autotoc_md117", null ],
        [ "2.2 CMake", "md_docs_2unit-testing.html#autotoc_md118", null ],
        [ "2.3 Unity", "md_docs_2unit-testing.html#autotoc_md119", null ],
        [ "2.4 Check the installation", "md_docs_2unit-testing.html#autotoc_md120", null ],
        [ "Linux", "md_docs_2unit-testing.html#autotoc_md121", null ]
      ] ],
      [ "3. Build and run", "md_docs_2unit-testing.html#autotoc_md123", [
        [ "Run one suite or see every test", "md_docs_2unit-testing.html#autotoc_md124", null ]
      ] ],
      [ "4. How a test program is compiled and linked", "md_docs_2unit-testing.html#autotoc_md126", null ],
      [ "5. How the mocks work", "md_docs_2unit-testing.html#autotoc_md128", null ],
      [ "6. Adding a test", "md_docs_2unit-testing.html#autotoc_md130", null ],
      [ "7. Troubleshooting", "md_docs_2unit-testing.html#autotoc_md132", null ]
    ] ],
    [ "VS Code code navigation (clangd) for ESP-IDF projects", "md_docs_2vscode-clangd-setup.html", [
      [ "How it fits together", "md_docs_2vscode-clangd-setup.html#autotoc_md135", null ],
      [ "Step 1: Install the clangd extension", "md_docs_2vscode-clangd-setup.html#autotoc_md137", null ],
      [ "Step 2: Make sure the extension is ENABLED", "md_docs_2vscode-clangd-setup.html#autotoc_md138", null ],
      [ "Step 3: Turn off Microsoft C/C++ IntelliSense", "md_docs_2vscode-clangd-setup.html#autotoc_md139", null ],
      [ "Step 4: Check the clangd binary and builtin headers exist", "md_docs_2vscode-clangd-setup.html#autotoc_md140", null ],
      [ "Step 5: Generate the compile database", "md_docs_2vscode-clangd-setup.html#autotoc_md141", null ],
      [ "Step 6: Workspace settings: <span class=\"tt\">.vscode/settings.json</span>", "md_docs_2vscode-clangd-setup.html#autotoc_md142", null ],
      [ "Step 7: Project file: <span class=\"tt\">.clangd</span> (committed)", "md_docs_2vscode-clangd-setup.html#autotoc_md143", null ],
      [ "Step 8: Restart clangd and verify", "md_docs_2vscode-clangd-setup.html#autotoc_md144", [
        [ "Optional: verify from the command line", "md_docs_2vscode-clangd-setup.html#autotoc_md145", null ]
      ] ],
      [ "Troubleshooting", "md_docs_2vscode-clangd-setup.html#autotoc_md147", null ],
      [ "After upgrading ESP-IDF or the tools", "md_docs_2vscode-clangd-setup.html#autotoc_md148", null ],
      [ "Using this setup in another ESP-IDF project", "md_docs_2vscode-clangd-setup.html#autotoc_md149", null ]
    ] ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"relay__gpio_8h.html#a839080f90ab81795a59e1b20f2dee2a8"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';