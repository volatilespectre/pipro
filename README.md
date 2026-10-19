# pipro (PIpeline PROfiler)
See what's happening inside your Unix pipes

## About
A CLI tool to make pipes more transparent when they're not working. Trying to run five different commands individually to see where the holdup is can be painful, so I made pipro to see exactly how long each individual process runs and what it returns.

### Features
- Wall-clock, user and system time, return code and memory usage reported
- Lightweight and portable: pipro has no third-party dependencies, and runs on Linux and BSDs (MacOS should work but I don't have the hardware to test it)
- Different levels of precision
- Catches errors and reports them at the end
- A cool animation to watch while you're bored waiting

### Usage
Pass the pipeline as a single quoted argument:
```bash
pipro "cmd1 | cmd2 | cmd3"
```

| Option | Description |
|---|---|
| `-s`, `--static` | Disable the interactive progress display and remove ANSI escape sequences |
| `-d`, `--decimals` | Set the number of decimal places (0-9) |
| `-h`, `--help` | Print a help message and exit |
| `-q`, `--quiet` | Suppress the pipeline's output |
Note that not all metrics can be reported to all levels, and high levels of precision may increase runtime due to frequent checks.

Example:
```bash
pipro -d 1 "seq 1 10000000 | sort -n | grep abc"
```
![A demonstration gif](./assets/demo.gif)

```bash
pipro -s -q "ping 8.8.8.8 | head"
Command name         Exit Status          Wall-Clock Time (s)  User CPU (s)    System CPU (s)  Peak memory usage (MB)
---------------------------------------------------------------------------------------------------------------------
head                 Returned 0 (OK)      8.059                0.000           0.008           7.914
ping                 Signal (13)          9.068                0.000           0.011           2.703
```

## Getting started

### Requirements
- A C compiler (GCC or Clang)
- Make

### Installation
Clone the repo and build using make:
```bash
git clone https://github.com/volatilespectre/pipro.git
cd pipro
sudo make install
```

## Roadmap
There's a number of feature ideas I haven't yet had time to implement. In no particular order:
- A test suite
- Disk usage tracking
- Test on MacOS

### Contributing
- If you find any bugs, you can report them through GitHub, please include steps to reproduce. 
- If you would like to see new features implemented, open an issue and I'll prioritise that if possible.
- If you have a Mac and can test it out, that would be a huge help.
- If you found pipro useful and want to help people access it, consider packaging it for your package manager of choice.

## License
Pipro is licensed under the GPL, v3 or later. See LICENSE for the full terms.