// RUN: %clang_cc1 %s -emit-llvm -triple i386-apple-darwin -o - | FileCheck %s
extern void abort();
extern void exit(int);
struct T
{
unsigned i:8;
unsigned c:24;
};
f(struct T t)
{
struct T s[1];
s[0]=t;
return(char)s->c;
}
main()
{
// CHECK:  getelementptr inbounds [1 x %struct.T]* %s, i32 0, i32 0
// CHECK:  getelementptr inbounds [1 x %struct.T]* %s, i32 0, i32 0
struct T t;
t.i=0xff;
t.c=0xffff11;
if(f(t)!=0x11)abort();
exit(0);
}
