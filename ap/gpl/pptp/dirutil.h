/* dirutil.h ... directory utilities.
 *               C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: dirutil.h,v 1.1.1.1 2000/12/23 08:19:51 scott Exp $
 */

#ifdef CODE_IN_USE  //Winster Chan added 05/16/2006
/* Returned malloc'ed string representing basename */
char *basenamex(char *pathname);
#endif  //CODE_IN_USE Winster Chan added 05/16/2006
/* Return malloc'ed string representing directory name (no trailing slash) */
char *dirname(char *pathname);
/* In-place modify a string to remove trailing slashes.  Returns arg. */
char *stripslash(char *pathname);
/* ensure dirname exists, creating it if necessary. */
int make_valid_path(char *dirname, mode_t mode);
