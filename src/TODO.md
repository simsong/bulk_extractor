- [ ] BE_DEBUG = print_steps to print the steps
<<<<<<< HEAD
- [ ] Make sure that process() and recurse() and anything else that's
  going to free an sbuf always get an sbuf with no parent, becuase
  otherwise the parent might be freed during the recursion, to allow
  for asynchronouse recursion.
=======
- [ ] Generic approach for setting the carve mode for a scanner.
- [ ] remove the file iterator from the image iterator, run it separately.
      Just put one file on the list, vs. many.
      map the entire file but process it with pages and margins using overlapping sbufs. Free it when finished?
>>>>>>> 844a7e610a4ab6632989c07d52c74b7195b4e24a
