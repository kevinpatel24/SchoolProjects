import stripe
from django.contrib.auth import authenticate, login as django_login, logout as django_logout
from django.contrib.auth.forms import UserCreationForm
from django.http import HttpResponse, HttpResponseRedirect
from django.shortcuts import render
from django.urls import reverse
from orders.forms import SignUpForm, AddNewProductForm
from .models import Category, Product, Review, OrderStatus, Order, OrderItem
from datetime import date
from django.contrib.auth.decorators import user_passes_test
from django.core.mail import send_mail
from pizza import settings

stripe.api_key = settings.STRIPE_SECRET_KEY



# Create your views here.
def index(request):
    if not request.user.is_authenticated:
        #if no logged in user found, take them to the login page
        return render(request, "login.html", {"message": None})
    context = {
        "user": request.user,
        "products": Product.objects.all()
    }
    return render(request,"project4index.html", context)

def login(request):
    username = request.POST["username"]
    password = request.POST["password"]
    user = authenticate(request, username=username, password=password)
    if user is not None:
        django_login(request, user)
        #login and redirect user to index
        return HttpResponseRedirect(reverse("index"))
    else:
        return render(request, "login.html", {"message": "Invalid credentials.", "color": "red"})

def logout(request):
    django_logout(request)
    return render(request, "login.html", {"message": "Logged out.", "color": "green"})

def register(request):
    if request.method == 'POST':
        #SignUpForm stored in forms.py
        form = SignUpForm(request.POST)
        if form.is_valid():
            form.save()
            username = form.cleaned_data.get('username')
            raw_password = form.cleaned_data.get('password1')
            user = authenticate(username=username, password=raw_password)
            django_login(request, user)
            return HttpResponseRedirect(reverse("index"))
    else:
        form = SignUpForm()
    return render(request, 'register.html', {'form': form})

def search(request):
    query = request.POST["query"]
    context = {
        "user": request.user,
        "products": Product.objects.filter(name__icontains=query)
    }
    return render(request,"project4index.html", context)

def product(request, productId):
    product = Product.objects.get(pk=productId)
    context = {
        "product": product,
        "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
        }
    return render(request, "product.html", context)

def review(request, productId):
    if request.method == 'POST':
        newReviewText = request.POST["review"]
        product = Product.objects.get(pk=productId)
        if len(newReviewText) == 0:
            context = {
            "message": "No review Text entered",
            "color": "red",
            "product": product,
            "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
            }
            return render(request, "product.html", context)
        else:
            try:
                #If review exists already, we only want to update the old review. Limit one review per item per customer.
                review = Review.objects.get(customer=request.user, product=Product.objects.get(pk=productId))
                review.reviewText = newReviewText
                review.reviewDate = date.today()
                review.save()
                context = {
                    "message": "Review Updated",
                    "color": "green",
                    "product": product,
                    "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
                    }
                return render(request, "product.html", context)
            except Review.DoesNotExist:
                try:
                    #Verify the user has bought the product before allowing them to review it. Otherwise deny review ability.
                    purchasedProduct = OrderItem.objects.get(product=Product.objects.get(pk=productId), order=Order.objects.filter(customer=request.user, orderStatus=OrderStatus.objects.get(status="Shipped"))[0])
                    newReview = Review(reviewText=newReviewText, product=product, customer=request.user, reviewDate=date.today())
                    newReview.save()
                    context = {
                        "message": "Review Added",
                        "color": "green",
                        "product": product,
                        "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
                        }
                    return render(request, "product.html", context)
                except OrderItem.DoesNotExist:
                    context = {
                        "message": "Cannot review a product you haven't purchased",
                        "color": "red",
                        "product": product,
                        "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
                        }
                    return render(request, "product.html", context)


def addToCart(request, productId):
    product = Product.objects.get(pk=productId)
    try:
        order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
    except Order.DoesNotExist:
        order = Order(customer=request.user, orderDate=date.today(), orderStatus=OrderStatus.objects.get(status="Unplaced"), address="N/A, Order still unplaced.")
        order.save()
    newOrderItem = OrderItem(product=product, order=order)
    newOrderItem.save()
    context = {
        "message": "Item added to Cart",
        "color": "green",
        "product": product,
        "reviews": Review.objects.filter(product=Product.objects.get(name=product.name))
        }
    return render(request, "product.html", context)

def removeFromCart(request,orderItemId):
    OrderItem.objects.get(pk=orderItemId).delete()
    context = {
        "user": request.user,
        "products": Product.objects.all(),
        "message": "Item removed from cart."
        }
    return render(request,"project4index.html", context)

def checkout(request):
    try:
        #find the unplaced order belonging to our customer
        order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
    except Order.DoesNotExist:
        context = {
            "user": request.user,
            "products": Product.objects.all(),
            "message": "Cart is empty!",
            "color": "red"
        }
        return render(request,"project4index.html", context)
    orderItems = OrderItem.objects.filter(order=order)
    totalPrice = 0
    for orderItem in orderItems:
        totalPrice = totalPrice + orderItem.product.price
    #For stripe, we need to send a string value in cents so we multiple our dollar value by 100 to account for this.
    stripePrice = totalPrice * 100
    context = {
    "orderItems": orderItems,
    "totalPrice": totalPrice,
    "stripePrice": stripePrice,
    "key": settings.STRIPE_PUBLISHABLE_KEY
    }
    return render(request, "checkout.html", context)

def placeOrder(request):
    if request.method == 'POST':
        order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
        orderItems = OrderItem.objects.filter(order=order)
        totalPrice = 0
        for orderItem in orderItems:
            totalPrice = totalPrice + orderItem.product.price
        #We multiple by 100 to adjust to cents and turn to an int because we need to send a value without decimals.
        stripePrice = totalPrice * 100
        stripePrice = int(stripePrice)
        charge = stripe.Charge.create(
            amount=stripePrice,
            currency='usd',
            description=order.pk,
            source=request.POST['stripeToken']
        )
        order.orderStatus = OrderStatus.objects.get(status="Pending")
        order.address = request.POST["address"]
        order.save()
        context = {
            "user": request.user,
            "products": Product.objects.all(),
            "message": "Order Placed!",
            "color": "green"
        }
        return render(request,"project4index.html", context)

@user_passes_test(lambda u: u.is_superuser)
def viewOrders(request):
    context= {
    "pendingOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Pending")),
    "completeOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Shipped"))
    }
    return render(request,"viewOrders.html", context)

@user_passes_test(lambda u: u.is_superuser)
def viewOrderDetails(request, orderId):
    order = Order.objects.get(pk=orderId)
    orderItems = OrderItem.objects.filter(order=order)
    context = {
    "order": order,
    "orderItems": orderItems
    }
    return render(request,"viewOrderDetails.html", context)

@user_passes_test(lambda u: u.is_superuser)
def markOrderComplete(request, orderId):
    #when superuser clicks a pending order, it is marked as completed
    order = Order.objects.get(pk=orderId)
    order.orderStatus = OrderStatus.objects.get(status="Shipped")
    order.save()
    customerEmail = order.customer.email
    #Sends email upon shipment to customer. We don't have an SMTP server set up so email is printed to console.
    send_mail(
    'Your order has shipped!',
    'Thank you for your order at the E-Store. Your order has shipped and will be with you soon. Enjoy!',
    request.user.email,
    [customerEmail],
    fail_silently=False,
)
    context= {
    "pendingOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Pending")),
    "completeOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Shipped"))
    }
    return render(request,"viewOrders.html", context)

@user_passes_test(lambda u: u.is_superuser)
def addNewItem(request):
    if request.method == 'POST':
        form = AddNewProductForm(request.POST)
        if form.is_valid():
            form.save()
            context = {
                "user": request.user,
                "products": Product.objects.all(),
                "message": "New Product Added.",
                "color": "green"
                }
            return render(request,"project4index.html", context)

    else:
        form = AddNewProductForm()
        return render(request, 'addNewItem.html', {'form': form})
