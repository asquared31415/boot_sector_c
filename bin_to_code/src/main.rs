#![feature(array_chunks)]

use std::{fmt::Write as _, fs, path::PathBuf};

use clap::{command, Parser};
use color_eyre::eyre::Result;
use eyre::bail;
use log::*;

#[derive(Debug, Parser)]
#[command()]
struct Args {
    filename: String,
    ptr_name: String,
}

fn main() -> Result<()> {
    color_eyre::install()?;
    simple_logger::SimpleLogger::new().init()?;

    let cli = Args::parse();
    let path = PathBuf::from(cli.filename.as_str()).canonicalize()?;
    info!("processing `{}`", path.display());

    let contents = fs::read(&path)?;
    if contents.len() % 2 != 0 {
        bail!("file length {} not a multiple of 2", contents.len());
    }

    let mut output = String::new();
    writeln!(&mut output, "  // BEGIN GENERATED CODE")?;

    let mut inc_count = 0;
    for (idx, &word) in contents.array_chunks::<2>().enumerate() {
        let val = u16::from_le_bytes(word);
        if val == 0 {
            inc_count += 1;
            continue;
        }
        if idx == 0 {
            write!(
                &mut output,
                "  * {ptr} = {val} ;\n",
                ptr = cli.ptr_name,
                val = val,
            )
            .expect("writing to string cannot fail");
        } else {
            write!(
                &mut output,
                "  {ptr} = {ptr} + {inc} ;\n  * {ptr} = {val} ;\n",
                ptr = cli.ptr_name,
                val = val,
                inc = inc_count
            )
            .expect("writing to string cannot fail");
        }

        inc_count = 1;
    }
    writeln!(&mut output, "  // END GENERATED CODE")?;

    fs::write("out/generated.txt", output)?;
    Ok(())
}
