#![feature(trait_upcasting)]

use clap::{command, Parser};
use color_eyre::eyre::Result;
use core::time::Duration;
use eyre::{bail, Context, Report};
use log::{debug, info};
use owo_colors::OwoColorize;
use serialport::SerialPort;
use std::{
    fs,
    io::{Read, Write},
    os::unix::{net::UnixStream, process::CommandExt},
    process::Command,
    thread,
};

#[derive(Parser)]
#[command()]
struct Args {
    #[arg(long)]
    socket: String,
    #[arg(long)]
    transfer_bin: String,
    #[arg(long)]
    no_socat: bool,
    #[arg(long)]
    no_checksum: bool,
}

enum Target {
    UnixSocket(UnixStream),
    SerialPort(Box<dyn SerialPort>),
}

impl Read for Target {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        match self {
            Target::UnixSocket(s) => s.read(buf),
            Target::SerialPort(s) => s.read(buf),
        }
    }
}

impl Write for Target {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        match self {
            Target::UnixSocket(s) => s.write(buf),
            Target::SerialPort(s) => s.write(buf),
        }
    }

    fn flush(&mut self) -> std::io::Result<()> {
        match self {
            Target::UnixSocket(s) => s.flush(),
            Target::SerialPort(s) => s.flush(),
        }
    }
}

fn main() -> Result<()> {
    env_logger::init();
    color_eyre::install()?;

    let cli = Args::parse();

    let text = format!("{} `{}`...", "connecting to".yellow(), cli.socket.red());
    info!("{}", text);

    // Assume that paths that start with unix-connect: are a socket
    let mut target = if let Some(socket) = cli.socket.strip_prefix("unix-connect:") {
        debug!("unix socket");
        Target::UnixSocket(UnixStream::connect(socket)?)
    } else {
        debug!("serial port device");
        Target::SerialPort(
            serialport::new(cli.socket.as_str(), 115_200)
                .timeout(Duration::from_millis(100))
                .open()
                .wrap_err_with(|| format!("failed to open serial port device `{}`", cli.socket))?,
        )
    };
    info!("{}", "Connected!".green());

    let mut target_bin = fs::read(cli.transfer_bin.as_str())
        .wrap_err_with(|| format!("could not open binary file `{}`", cli.transfer_bin))?;
    target_bin.push(0x00); // programs terminate with a NULL byte

    do_write(&mut target, target_bin.as_slice())?;

    if !cli.no_checksum {
        let mut checksum = 0_u16;
        target_bin.iter().for_each(|&b| {
            checksum = checksum.wrapping_add(b as u16);
        });

        let mut checksum_buf = [0_u8; 2];
        target.read_exact(&mut checksum_buf)?;

        let recv_checksum = u16::from_le_bytes(checksum_buf);
        if recv_checksum == checksum {
            info!("{}", "checksum passed".green());
        } else {
            bail!(
                "{} expected {:#06X}, found {:#06X}",
                "checksum did not match!".red(),
                checksum,
                recv_checksum
            );
        }
    }

    info!("{}", "Done!".green());

    if !cli.no_socat {
        let mut socat_target = cli.socket.clone();
        if matches!(target, Target::SerialPort(_)) {
            socat_target.push_str(",b115200,rawer");
        }

        let mut cmd = Command::new("socat");
        cmd.args(["-,raw", socat_target.as_str()]);

        // Does not return on success
        return Err(Report::from(cmd.exec()));
    }

    Ok(())
}

const TRANSFER_DELAY: Duration = Duration::from_micros(200);

fn do_write<T: Read + Write>(target: &mut T, bin: &[u8]) -> Result<(), std::io::Error> {
    for (idx, &b) in bin.iter().enumerate() {
        if idx % 0x80 == 0 {
            info!("transferring... {:#06X}/{:#06X}", idx, bin.len());
        }
        target.write(&[b])?;
        let _ = target.flush();
        // Sleep a little so that it reads properly.
        thread::sleep(TRANSFER_DELAY);
    }
    let _ = target.flush();
    info!("transferring... {:#06X}/{:#06X}", bin.len(), bin.len());
    Ok(())
}
