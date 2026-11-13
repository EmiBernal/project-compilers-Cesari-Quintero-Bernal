typedef int boolean;
#define true 1
#define false 0
#define main ctds_main
int a;
int b;
boolean c;

int main() {
    a = 150;
    b = 200;
    c = true;
    if (c) {
        a = a + b;
    }
    return a;
}
