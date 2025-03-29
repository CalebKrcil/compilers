// This is a comment before the program

/*
   Multi-line comment before the program
*/

fun main() {
    // Variable Declarations
    val a: Int = 10
    var b: Int = 5
    var c: Int
    c = a + b  // Assignment and arithmetic expressions

    // Print Statements
    println("Hello, k0!")
    println("Sum: $c")

    // If-Else Statements
    if (c > 10) {
        println("c is greater than 10")
    } else if (c == 10) {
        println("c is exactly 10")
    } else {
        println("c is less than 10")
    }

    // While Loop
    var counter = 3
    while (counter > 0) {
        println("Countdown: $counter")
        counter = counter - 1
    }


    // Kotlin-Style For-In Loop
    for (i in 1..3) {
        println("For Loop Kotlin: $i")
    }

    // Function Calls
    val squared = square(4)
    println("Squared: $squared")

    // Boolean Expressions & Logical Operators
    val x = true
    val y = false
    if (x && !y) {
        println("Boolean logic works!")
    }
    // Function with Multiple Parameters
    println("Sum Function: " + sum(3, 4))

    // Nested Conditionals
    if (a > 0) {
        if (b > 0) {
            println("Both a and b are positive")
        }
    }
}

// Function Definitions
fun square(n: Int): Int {
    return n * n
}

fun sum(x: Int, y: Int): Int {
    return x + y
}

// This is a comment after the program

/*
   Multi-line comment at the end of the file
*/
