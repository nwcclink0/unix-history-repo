/* pututline.c: A replacement for the pututline() function

%%% copyright-cmetz
This software is Copyright 1996 by Craig Metz, All Rights Reserved.
The Inner Net License Version 2 applies to this software.
You should have received a copy of the license with this software. If
you didn't get a copy, you may request one from <license@inner.net>.

        History:

	Created by cmetz for OPIE 2.3.
*/

#include "opie_cfg.h"
#include <stdio.h>
#include <utmp.h>
#include "opie.h"

void pututline FUNCTION((utmp), struct utmp *utmp)
{
  FILE *f;
  struct utmp u;
  int i;

  if (!(f = __opieopen(_PATH_UTMP, 1, 0644)))
    return;

#if HAVE_TTYSLOT
  if (i = ttyslot()) {
    if (fseek(f, i * sizeof(struct utmp), SEEK_SET) < 0)
      goto ret;
    fwrite(utmp, sizeof(struct utmp), 1, f);
    goto ret;
  }
#endif /* HAVE_TTYSLOT */

  while(fread(&u, sizeof(struct utmp), 1, f) == sizeof(struct utmp)) {
    if (!strncmp(utmp->ut_line, u.ut_line, sizeof(u.ut_line) - 1)) {
      if ((i = ftell(f)) < 0)
        goto ret;
      if (fseek(f, i - sizeof(struct utmp), SEEK_SET) < 0)
        goto ret;
      fwrite(utmp, sizeof(struct utmp), 1, f);
      goto ret;
    }
  }

  fclose(f);

  if (!(f = __opieopen(_PATH_UTMP, 2, 0644)))
    return;
  fwrite(utmp, sizeof(struct utmp), 1, f);

ret:
  fclose(f);
}
