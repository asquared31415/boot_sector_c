#![feature(os_str_display)]

use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::LazyLock;

use clap::{Parser, Subcommand};
use eyre::Result;
use eyre::{Context, bail};
use nix::mount::{MsFlags, mount, umount};
use regex::Regex;
use walkdir::WalkDir;

#[derive(Debug, Parser)]
#[command(version)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Debug, Subcommand)]
enum Commands {
    IdentDump {
        #[arg()]
        path: PathBuf,
    },
    SyncPrograms {
        #[arg()]
        qemu_img: PathBuf,
        #[arg()]
        programs: PathBuf,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();

    match &cli.command {
        Commands::IdentDump { path } => do_ident_dump(&path),
        Commands::SyncPrograms { qemu_img, programs } => sync_programs(qemu_img, &programs),
    }
}

static ITEM_START: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r"^(?P<ty>int|int\*|void) (?P<sym>\w+)").unwrap());

fn do_ident_dump(path: &Path) -> Result<()> {
    let contents = fs::read_to_string(&path)
        .wrap_err_with(|| format!("could not open file `{}`", path.display()))?;

    let mut map = BTreeMap::<u16, (usize, String)>::new();

    for (idx, line) in contents.lines().enumerate() {
        if let Some(captures) = ITEM_START.captures(line) {
            // let ty = captures.name("ty").unwrap().as_str();
            let sym = captures.name("sym").unwrap().as_str();
            let num = ident_to_num(sym);

            map.insert(num, (idx + 1, sym.to_string()));
        }
    }

    for (num, (line, sym)) in map {
        println!("{:#06X}: {:>24} L{:>4}", num, sym, line);
    }

    Ok(())
}

fn ident_to_num(ident: &str) -> u16 {
    ident.bytes().fold(0_u16, |acc, b| {
        acc.wrapping_mul(10)
            .wrapping_add(u16::from(b))
            .wrapping_sub(u16::from(b'0'))
    })
}

fn sync_programs(qemu_img: &Path, path: &Path) -> Result<()> {
    let loop_output = Command::new("losetup")
        .args([
            "--partscan",
            "--direct-io=on",
            "--nooverlap",
            "--show",
            "--find",
            &format!("{}", qemu_img.canonicalize()?.display()),
        ])
        .output()?;
    if !loop_output.status.success() {
        bail!("{}", String::from_utf8_lossy(&loop_output.stderr));
    }
    let mut loop_dev = String::from_utf8(loop_output.stdout)?.trim().to_string();
    // TODO: have a way to specify partition?
    loop_dev.push_str("p1");

    let target = PathBuf::from("/mnt/qemu");

    mount(
        Some(loop_dev.as_str()),
        &target,
        Some("vfat"),
        MsFlags::empty(),
        Some("umask=0000"),
    )
    .wrap_err("mount failed")?;

    for entry in WalkDir::new(path)
        .same_file_system(true)
        .into_iter()
        .filter_map(|e| e.ok())
    {
        if entry.file_type().is_file() {
            // TODO: add directory support
            let name = entry.file_name().to_string_lossy();
            println!("syncing {}", name);

            fs::write(
                target.join(&Path::new(entry.file_name())),
                fs::read(entry.path()).unwrap(),
            )
            .unwrap();
        }
    }

    // cleanup
    let mount_res = umount("/mnt/qemu");
    let loop_res = Command::new("losetup")
        .arg("-D")
        .spawn()
        .and_then(|mut c| c.wait());
    // we make sure to run both commands before propagating errors
    mount_res?;
    loop_res?;

    Ok(())
}
