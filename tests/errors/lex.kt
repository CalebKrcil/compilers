
fun main(args: Array<String>) {
    val a = "Unclosed string

    val b = @42
    val c = #12
    val d = `backticks`

    val 3startsWithDigit = 5
    val hello-world = 10

    /* not closed

    val badEscape1 = "Invalid: \q"
    val badEscape2 = "\x"
    val badEscape3 = "\u12G4"

    val multiChar = 'xy'
    val emptyChar = ''
    val badEscapeChar = '\q'

    val path = "C:\new\file\folder\"

    val badHex = 0xZZ
    val badBin = 0b120
    val tooManyDots = 1.2.3

    val class = 10
    val fun = 5
    val when = "keyword"

    val dynamic = 42
    val suspend = false

    println(a + b + c + d + badEscape1 + multiChar + path + badHex + class + dynamic)
}