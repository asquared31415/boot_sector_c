fn main() {
    const IDENT: &str = "return;";

    let mut val = 0_u16;
    for &b in IDENT.as_bytes() {
        val = val.wrapping_mul(10);
        val = val.wrapping_add(u16::from(b));
        val = val.wrapping_sub('0' as u16);
    }

    println!("{}: {:#06X}", IDENT, val);
}
