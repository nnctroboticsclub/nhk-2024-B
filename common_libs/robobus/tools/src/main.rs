use clap::Parser;
#[derive(Parser)]
#[command(version, about)]
struct Args {}

fn main() {
    let args = Args::parse();

    println!("Hello, world!");
}
