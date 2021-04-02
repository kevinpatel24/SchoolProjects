# Generated by Django 2.2.7 on 2019-11-10 22:14

from django.db import migrations, models
import django.db.models.deletion


class Migration(migrations.Migration):

    initial = True

    dependencies = [
    ]

    operations = [
        migrations.CreateModel(
            name='AddOn',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=20, unique=True)),
            ],
        ),
        migrations.CreateModel(
            name='AddOnType',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=20, unique=True)),
            ],
        ),
        migrations.CreateModel(
            name='Category',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=20, unique=True)),
            ],
        ),
        migrations.CreateModel(
            name='MenuItem',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('price', models.DecimalField(decimal_places=2, max_digits=4)),
                ('maxAddOns', models.IntegerField()),
                ('addOnType', models.ForeignKey(blank=True, on_delete=django.db.models.deletion.CASCADE, to='orders.AddOnType')),
                ('category', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='orders.Category')),
            ],
        ),
        migrations.CreateModel(
            name='Order',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('customer', models.CharField(max_length=64)),
                ('orderDate', models.DateField()),
            ],
        ),
        migrations.CreateModel(
            name='OrderStatus',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('status', models.CharField(max_length=20, unique=True)),
            ],
        ),
        migrations.CreateModel(
            name='Size',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('name', models.CharField(max_length=10, unique=True)),
            ],
        ),
        migrations.CreateModel(
            name='OrderItem',
            fields=[
                ('id', models.AutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('total', models.DecimalField(decimal_places=2, max_digits=6)),
                ('addOns', models.ManyToManyField(to='orders.AddOn')),
                ('menuItem', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='orders.MenuItem')),
                ('order', models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='orders.Order')),
            ],
        ),
        migrations.AddField(
            model_name='order',
            name='orderStatus',
            field=models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='orders.OrderStatus'),
        ),
        migrations.AddField(
            model_name='menuitem',
            name='size',
            field=models.ForeignKey(blank=True, on_delete=django.db.models.deletion.CASCADE, to='orders.Size'),
        ),
        migrations.AddField(
            model_name='addon',
            name='addOnType',
            field=models.ForeignKey(on_delete=django.db.models.deletion.CASCADE, to='orders.AddOnType'),
        ),
    ]