#!/usr/bin/env kotlin

fun main() {
    // comment
    /*
        multiline followed by whitespace and newline
    */
    


    val a: Int = 10;
    val b: Float = 10.10;
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

    // Demonstrating variable declarations
    val constantValue: Int = 10
    var mutableVariable: Int = 5

    // Arithmetic operators
    val addition = constantValue + mutableVariable
    val subtraction = constantValue - mutableVariable
    val multiplication = constantValue * mutableVariable
    val division = constantValue / mutableVariable
    val modulus = constantValue % mutableVariable

    // Increment and decrement
    mutableVariable++
    mutableVariable--

    // Comparison operators
    val isLessThan = constantValue < mutableVariable
    val isGreaterThan = constantValue > mutableVariable
    val isLessOrEqual = constantValue <= mutableVariable
    val isGreaterOrEqual = constantValue >= mutableVariable

    // Equality operators
    val isEqual = (constantValue == mutableVariable)
    val isStrictEqual = (constantValue === mutableVariable)
    val isNotEqual = (constantValue != mutableVariable)

    // Logical operators
    val trueValue = true
    val falseValue = false
    val conjunction = trueValue && falseValue
    val disjunction = trueValue || falseValue
    val negation = !trueValue

    // Control flow
    if (constantValue > 0) {
        println("Positive value")
    } else {
        println("Non-positive value")
    }

    // Loop
    for (i in 0..5) {
        when (i) {
            0 -> println("Zero")
            in 1..3 -> println("Between 1 and 3")
            else -> println("Greater than 3")
        }

        if (i == 3) {
            break
        }
    }

    // Function with arrow notation
    val lambda = { x: Int -> x * 2 }
    println(lambda(4))

    // Nullable and safe call
    val nullableValue: String? = null
    println(nullableValue?.length)

    // Import and const (demonstrating these keywords)
    println("Lexer symbol demonstration complete!")
}