/*
 * Unix SMB/CIFS implementation. 
 * Copyright (C) Jeremy Allison 1995-1998
 * Copyright (C) Tim Potter     2001
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 675
 * Mass Ave, Cambridge, MA 02139, USA.  */

#include "includes.h"

#if 1
  #define P1MSG(args...)
#else
  #define P1MSG(args...) fprintf(stderr, "%s-%04d: ", __FILE__, __LINE__) ; fprintf(stderr, ## args)
#endif // NDEBUG
#if 0
//ori: /lib/util_unistr.c
/*******************************************************************
 Count the number of characters in a smb_ucs2_t string.
********************************************************************/
size_t strlen_w(const smb_ucs2_t *src)
{
    size_t len;

    for(len = 0; *src++; len++) ;

    return len;
}

//ori: smbencrypt.c
/**
 * Creates the DES forward-only Hash of the users password in DOS ASCII charset
 * @param passwd password in 'unix' charset.
 * @param p16 return password hashed with DES, caller allocated 16 byte buffer
 * @return False if password was > 14 characters, and therefore may be incorrect, otherwise True
 * @note p16 is filled in regardless
 */
 
BOOL E_deshash(const char *passwd, uchar p16[16])
{
    BOOL ret = True;
    fstring dospwd; 
    ZERO_STRUCT(dospwd);
    
    /* Password must be converted to DOS charset - null terminated, uppercase. */
    //push_ascii(dospwd, passwd, sizeof(dospwd), STR_UPPER|STR_TERMINATE);
    {
        int i;
    
        if (passwd == 0)
            return 0;
        i = 0;
        while(1)
        {
            if(passwd[i] == 0)
            {
                dospwd[i] = 0;
                break;
            }
            else
            {
                dospwd[i] = toupper(passwd);
            }
            i++;
        }
    }
    
    /* Only the fisrt 14 chars are considered, password need not be null terminated. */
    E_P16((const unsigned char *)dospwd, p16);

    if (strlen(dospwd) > 14) {
        ret = False;
    }

    ZERO_STRUCT(dospwd);    

    return ret;
}

//ori: smbencrypt.c
/**
 * Creates the MD4 Hash of the users password in NT UNICODE.
 * @param passwd password in 'unix' charset.
 * @param p16 return password hashed with md4, caller allocated 16 byte buffer
 */
 
void E_md4hash(const char *passwd, uchar p16[16])
{
    int len;
    smb_ucs2_t wpwd[129];
    
    /* Password must be converted to NT unicode - null terminated. */
    //push_ucs2(NULL, wpwd, (const char *)passwd, 256, STR_UNICODE|STR_NOALIGN|STR_TERMINATE);
    /* Calculate length in bytes */
    len = strlen_w(wpwd) * sizeof(int16);

    mdfour(p16, (unsigned char *)wpwd, len);
    ZERO_STRUCT(wpwd);    
}

#endif //if 0
#define SAMBA_NO_PASSWD "NO PASSWORDXXXXXXXXXXXXXXXXXXXXX"
#define SAMBA_CTL_PWNOTREQ "[NU         ]"
#define SAMBA_CTL_NORMAL "[U          ]"

/*********************************************************
 Start here.
**********************************************************/
int main(int argc, char **argv)
{    

    int i;
    uchar new_lanman_p16[LM_HASH_LEN];
    uchar new_nt_p16[NT_HASH_LEN];
    char lanPW[LM_HASH_LEN * 2 + 1];
    char ntPW[NT_HASH_LEN * 2 + 1];
    char *tmp;
    FILE *fp;

    if (argc == 2 || argc == 3 ) 
    {
        fp  = fopen("/usr/local/samba/private/smbpasswd", "a");

        if (argc == 3)
        {
        //==================
        //create hashs
        //==================
            memset(lanPW, 0, LM_HASH_LEN * 2 + 1);
            memset(ntPW, 0, NT_HASH_LEN * 2 + 1);
            E_md4hash(argv[2], new_nt_p16);

            for(i=0; i<NT_HASH_LEN; i++)
            {
                asprintf(&tmp, "%.2X", new_nt_p16[i]);
                ntPW[i*2] = tmp[0];
                ntPW[i*2 + 1] = tmp[1];
                free(tmp);
            }

            if (!E_deshash(argv[2], new_lanman_p16)) {
                fprintf(stderr, "E_deshash failed\n");
                return -1;
            }
    
            for(i=0; i<LM_HASH_LEN; i++)
            {
                asprintf(&tmp, "%.2X", new_lanman_p16[i]);
                lanPW[i*2] = tmp[0];
                lanPW[i*2 + 1] = tmp[1];
                free(tmp);
            }
        //==================
        //save to smbpasswd
        //==================
            P1MSG("save \"%s:0:%s:%s:%s:LCT-3E12A0AC:\"\n", argv[1], lanPW, ntPW, SAMBA_CTL_NORMAL);
            if (fp) 
            {
                fprintf(fp, "%s:0:%s:%s:%s:LCT-3E12A0AC:\n", argv[1], lanPW, ntPW, SAMBA_CTL_NORMAL);
            }
            else
            {
                perror("open smb file faild: ");            
            }
        } //if (argc == 3)
        else 
        {
        //======================
        //arg == 2, no password
        //======================
            P1MSG("save \"%s:0:%s:%s:%s:LCT-3E12A0AC:\"\n", argv[1], SAMBA_NO_PASSWD, SAMBA_NO_PASSWD, SAMBA_CTL_PWNOTREQ);
            if (fp) 
            {
                fprintf(fp, "%s:0:%s:%s:%s:LCT-3E12A0AC:\n", argv[1], SAMBA_NO_PASSWD, SAMBA_NO_PASSWD, SAMBA_CTL_PWNOTREQ);
            }
            else
            {
                perror("open smb file faild: ");            
            }
        } // agc == 2. 
             
        if (fp) 
        {
           fclose(fp);
        }
    }
    else
    {
        printf("usage smb_pass user passwd\n");
    }
    return 0;
}
