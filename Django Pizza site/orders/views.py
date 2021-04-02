from django.contrib.auth import authenticate, login as django_login, logout as django_logout
from django.contrib.auth.forms import UserCreationForm
from django.http import HttpResponse, HttpResponseRedirect
from django.shortcuts import render
from django.urls import reverse
from orders.forms import SignUpForm
from .models import MenuItem, Category, AddOnType, AddOn, Size, OrderStatus, Order, OrderItem, AddOnCount
from datetime import date
from django.contrib.auth.decorators import user_passes_test


# Create your views here.
def index(request):
    if not request.user.is_authenticated:
        #if no logged in user found, take them to the login page
        return render(request, "login.html", {"message": None})
    context = {
        "user": request.user,
        "menuItems": MenuItem.objects.all()
    }
    return render(request,"project3index.html", context)

def login(request):
    username = request.POST["username"]
    password = request.POST["password"]
    user = authenticate(request, username=username, password=password)
    if user is not None:
        django_login(request, user)
        #login and redirect user to index
        return HttpResponseRedirect(reverse("index"))
    else:
        return render(request, "login.html", {"message": "Invalid credentials."})

def logout(request):
    django_logout(request)
    return render(request, "login.html", {"message": "Logged out."})

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

def customize(request, menuItem_id):
    menuItem = MenuItem.objects.get(pk=menuItem_id)
    context = {
        "menuItem": menuItem,
        "maxAddOns": range(0,menuItem.maxAddOns),
        "customizationOptions": AddOn.objects.filter(addOnType=menuItem.addOnType)
    }
    return render(request, "customize.html", context)


def addToCart(request, menuItem_id):
    menuItem = MenuItem.objects.get(pk=menuItem_id)
    maxAddOns = menuItem.maxAddOns
    menuItemPrice = menuItem.price
    menuItemCategory = menuItem.category
    menuItemSize = menuItem.size
    menuItemAddOnType = menuItem.addOnType
    #try to find the unplaced order(cart) belonging to this customer. If none is found, create a new one
    try:
        order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
    except Order.DoesNotExist:
        order = Order(customer=request.user, orderDate=date.today(), orderStatus=OrderStatus.objects.get(status="Unplaced"))
        order.save()
    #create a new blank list where we will store all the addOnIds that the user choose
    addOnIds = []
    for i in range(1,6):
        if ("addOnSelection"+str(i)) in request.POST:
            addOnIds.append(int(request.POST["addOnSelection"+str(i)]))
    #get rid of all duplicate customization choices.
    addOnIds = set(addOnIds)
    #We have three None addOns of slightly seperate names because we have three addOnTypes. We get rid of them from the list so the customer is not charged for a None topping
    while AddOn.objects.get(name="None").pk in addOnIds:
        addOnIds.remove(AddOn.objects.get(name="None").pk)

    while AddOn.objects.get(name="None.").pk in addOnIds:
        addOnIds.remove(AddOn.objects.get(name="None.").pk)

    while AddOn.objects.get(name="None!").pk in addOnIds:
        addOnIds.remove(AddOn.objects.get(name="None!").pk)
    #calculate the price of our addOns and then the total price
    addOnPrice = AddOnCount.objects.get(numberOfAddOns=len(addOnIds), category=Category.objects.get(name=menuItemCategory), size=Size.objects.get(name=menuItemSize), addOnType=AddOnType.objects.get(name=menuItemAddOnType)).price
    itemPrice = addOnPrice + menuItemPrice
    newOrderItem = OrderItem(menuItem=menuItem, order=order, total=itemPrice)
    newOrderItem.save()
    for addOnId in addOnIds:
        newOrderItem.addOns.add(AddOn.objects.get(pk=addOnId))
    newOrderItem.save()
    context = {
        "user": request.user,
        "menuItems": MenuItem.objects.all(),
        "message": "Item Added to Cart!"
    }
    return render(request,"project3index.html", context)
    #is it possible to use reverse and send message?
    #return HttpResponseRedirect(reverse("index", kwargs={'message': 'Item added to cart'}))


def checkout(request):
    try:
        #find the unplaced order belonging to our customer
        order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
    except Order.DoesNotExist:
        context = {
            "user": request.user,
            "menuItems": MenuItem.objects.all(),
            "message": "No items in Cart!"
        }
        return render(request,"project3index.html", context)
    orderItems = OrderItem.objects.filter(order=order)
    totalPrice = 0
    for orderItem in orderItems:
        totalPrice = totalPrice + orderItem.total
    context = {
    "orderItems": orderItems,
    "totalPrice": totalPrice
    }
    return render(request, "checkout.html", context)

def placeOrder(request):
    #find the unplaced order and change the status of it to pending
    order = Order.objects.get(customer=request.user, orderStatus=OrderStatus.objects.get(status="Unplaced"))
    order.orderStatus = OrderStatus.objects.get(status="Pending")
    order.save()
    context = {
        "user": request.user,
        "menuItems": MenuItem.objects.all(),
        "message": "Order Placed!"
    }
    return render(request,"project3index.html", context)

@user_passes_test(lambda u: u.is_superuser)
def viewOrders(request):
    context= {
    "pendingOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Pending")),
    "completeOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Complete"))
    }
    return render(request,"viewOrders.html", context)

@user_passes_test(lambda u: u.is_superuser)
def viewOrderDetails(request, order_id):
    order = Order.objects.get(pk=order_id)
    orderItems = OrderItem.objects.filter(order=order)
    context = {
    "order": order,
    "orderItems": orderItems
    }
    return render(request,"viewOrderDetails.html", context)

@user_passes_test(lambda u: u.is_superuser)
def markOrderComplete(request, order_id):
    #when superuser clicks a pending order, it is marked as completed
    order = Order.objects.get(pk=order_id)
    order.orderStatus = OrderStatus.objects.get(status="Complete")
    order.save()
    context= {
    "pendingOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Pending")),
    "completeOrders": Order.objects.filter(orderStatus=OrderStatus.objects.get(status="Complete"))
    }
    return render(request,"viewOrders.html", context)
