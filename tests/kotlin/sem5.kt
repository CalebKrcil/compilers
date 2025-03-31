sealed class Result
class Success(val value: String) : Result()
class Error(val message: String) : Result()
fun main() {
    val result: Result = Success("Done")
    when (result) {
        is Success -> println(result.value)
        is Error -> println(result.message)
    }
}