#!/usr/bin/env kotlin

fun main() {
    // comment
    /*
        multiline followed by whitespace and newline
    */
    


    val a: Int = 10
    val b: Float = 10.10
    var c: Char = '1'
    val d: String = "wow"
    val e: Int = -10
    val f: Float = -10.10
    val g: Boolean = true
    val h: Boolean = false
    val i: String = """
    string
    """

    for (i in 1..5) {
        if (i == 3) {
            break
        }
        println(i)
    }

    for (i in 1..5) {
        if (i == 3) {
            continue
        }
        println(i)
    }

    var i = 0
    do {
        println(i)
        i++
    } while (i < 3)

    val condition = true
    if (condition) {
        println("True")
    } else {
        println("False")
    }

    for (i in 1..3) {
        println(i)
    }

    const val CONSTANT_VALUE: Int = 100
    println("Constant: $CONSTANT_VALUE")
}