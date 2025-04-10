// ===== TYPE MATCHING =====
val a: Int = 10
val b: Double = 3.14
val c: Boolean = true
val d: Char = 'z'
val e: String = "hello"

// ===== TYPE MISMATCH (should fail) =====
// val bad1: Int = "nope"
// val bad2: Boolean = 123

// ===== OPERATOR USAGE =====
val sum: Int = 1 + 2
val product: Double = 2.0 * 4.5
val truth: Boolean = true && false
val order: Boolean = 5 < 6

// ===== OPERATOR MISMATCH (should fail) =====
//val oops1: Int = true + 1
val oops2: Boolean = c && d

val str1: String = "hello, "
val str2: String = "world!"
val combined: String = str1 + str2
val str3: String = str1 + 25

// ===== FUNCTION DECLARATIONS AND CALLS =====
fun add(x: Int, y: Int, z: Int): Int {
    return x + y + z
}
val result: Int = add(4, 5, a)

// ===== FUNCTION CALL MISMATCH (should fail) =====
// val failFunc: Boolean = add(1, 3)

// ===== MUTABILITY CHECKING =====
fun main(){
    var counter: Int
    counter = 13 // ✅ okay
    val constVal: Int = 10
    // constVal = 20 // ❌ should fail (val is immutable)

// ===== NULLABILITY =====
val maybeInt: Int = 13
val alsoMaybe: String? = null
// val nonNullable: Int = null // ❌ should fail
// val notMaybe: Boolean = null // ❌ should fail

// ===== BUILT-IN FUNCTION CALLS =====
println("Hello from k0")
println(result)

// ===== ARRAY TESTS =====
var data: Array<Int>(8) {0}
val first: Int = data[0]
// val badArr: Double = arr[0] // ❌ if expecting Double, but it's Int

}

// ===== NESTED EXPRESSIONS / DOT CHAINS =====
fun timesTwo(x: Int): Int {
    return x * 2
}
val z: Int = timesTwo(add(1, 2, a)) + 5

// ===== NESTED FUNCTION DECLARATION (should fail) =====
// fun wrapper() {
//     fun nested() { println("illegal") }
// }