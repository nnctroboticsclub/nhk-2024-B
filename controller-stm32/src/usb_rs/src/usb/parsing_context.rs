use super::EP0;

pub struct ParsingContext<'a, T: EP0> {
    pub ep0: &'a mut T,
}
