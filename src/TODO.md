- [ ] BE_DEBUG = print_steps to print the steps
- [ ] Make sure that process() and recurse() and anything else that's
  going to free an sbuf always get an sbuf with no parent, becuase
  otherwise the parent might be freed during the recursion, to allow
  for asynchronouse recursion.
