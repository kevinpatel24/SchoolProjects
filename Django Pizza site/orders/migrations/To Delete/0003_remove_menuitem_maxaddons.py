# Generated by Django 2.2.7 on 2019-11-10 23:49

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('orders', '0002_menuitem_name'),
    ]

    operations = [
        migrations.RemoveField(
            model_name='menuitem',
            name='maxAddOns',
        ),
    ]
