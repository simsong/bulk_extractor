SOURCE: https://hunnysoft.com/mime/samples/index.html

The files in this directory that match the pattern m*.txt are sample
messages in MIME format from common email Mail User Agents (MUAs).

I created these samples mainly for the purpose of testing my own MIME
implementation, particularly for testing the decoding of encoded text
in the headers (RFC 2047) and for the extraction of attachments (not
covered by any standard).

I used four different MUAs to create the messages.  You can determine which
MUA was used by looking at the file name.  The breakdown is:

   m0*.eml  -- Microsoft Outlook 00
   m1*.eml  -- Netscape Communicator 4.7
   m2*.eml  -- Qualcomm Eudora 4.2
   m3*.eml  -- PINE (Linux)

Of course, you can also look at the headers of the message to determine the
MUA.

If you want to contribute to this collection of samples, please do send
your contributions.  I will evaluate the contributions for inclusion in
this collection.  The evaluation criteria will include:

  * Is the MUA a mainstream MUA (not some obscure, rare MUA)?

  * Are the messages short?

  * Are the messages useful for testing?

  * Are the messages genuine?  Messages that have been through a relaying
    SMTP server might have been modified.  This might not disqualify a
    sample message, as it probably still has value for interoperability
    testing.

When you send messages, please zip them or tar them, so that they won't be
changed as they pass through the mail system.  If I include your messages,
I will put your name on a list of contributors, unless you prefer otherwise.

I am also considering creating a separate collection of messages designed
to stress test MIME implementations.  So, if you have any good examples of
bad messages (but not bad examples of good messages :-), feel free to send
them.

Not all the messages are correct to the MIME standard.  However, since
these messages are from popular MUAs, they can be useful for
interoperability testing.  Please, if you are creating messages, read and
understand the standards documents, rather than imitate what you see in
these messages!

I have also included a simple Java program that I used for creating these
samples.  This program, SmtpServer.java, is a simple SMTP server that will
receive the message directly from you MUA.  This is important, because if
you just route the message through your normal SMTP server, the server
might make changes to the message.  For example, it seems to be common for
some servers to convert quoted-printable encoded text to 8-bit text.[1]
This SMTP server program records the entire SMTP client/server dialog,
which is great if you are trying to debug your mail system, but it also
means that you will have to edit the output of the program to get just
the email message.

The URL for this collection of messages is
<http://hunnysoft.com/mime/samples.zip>





----------------------------------------------------

[1] I don't like the fact that this happens, but that's life.  I guess the
MTA thinks that since it can handle 8-bit text, that quoted-printable
encoding is not necessary.  However, quoted-printable also makes long lines
into short lines, which is something other than converting to 8-bit
characters.  In general, I think end-to-end transparency is a good thing;
anything that interferes with transparency should be avoided.
